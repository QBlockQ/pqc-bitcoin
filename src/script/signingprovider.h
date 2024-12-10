// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGNINGPROVIDER_H
#define BITCOIN_SCRIPT_SIGNINGPROVIDER_H

#include <addresstype.h>
#include <attributes.h>
#include <key.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <script/script.h>
#include <sync.h>
#include <crypto/pqc/hybrid_key.h>

struct ShortestVectorFirstComparator
{
    bool operator()(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) const
    {
        if (a.size() < b.size()) return true;
        if (a.size() > b.size()) return false;
        return a < b;
    }
};

struct TaprootSpendData
{
    /** The BIP341 internal key. */
    XOnlyPubKey internal_key;
    /** The Merkle root of the script tree (0 if no scripts). */
    uint256 merkle_root;
    /** Map from (script, leaf_version) to (sets of) control blocks.
     *  More than one control block for a given script is only possible if it
     *  appears in multiple branches of the tree. We keep them all so that
     *  inference can reconstruct the full tree. Within each set, the control
     *  blocks are sorted by size, so that the signing logic can easily
     *  prefer the cheapest one. */
    std::map<std::pair<std::vector<unsigned char>, int>, std::set<std::vector<unsigned char>, ShortestVectorFirstComparator>> scripts;
    /** Merge other TaprootSpendData (for the same scriptPubKey) into this. */
    void Merge(TaprootSpendData other);
};

/** Utility class to construct Taproot outputs from internal key and script tree. */
class TaprootBuilder
{
private:
    /** Information about a tracked leaf in the Merkle tree. */
    struct LeafInfo
    {
        std::vector<unsigned char> script;   //!< The script.
        int leaf_version;                    //!< The leaf version for that script.
        std::vector<uint256> merkle_branch;  //!< The hashing partners above this leaf.
    };

    /** Information associated with a node in the Merkle tree. */
    struct NodeInfo
    {
        /** Merkle hash of this node. */
        uint256 hash;
        /** Tracked leaves underneath this node (either from the node itself, or its children).
         *  The merkle_branch field of each is the partners to get to *this* node. */
        std::vector<LeafInfo> leaves;
    };
    /** Whether the builder is in a valid state so far. */
    bool m_valid = true;

    /** The current state of the builder.
     *
     * For each level in the tree, one NodeInfo object may be present. m_branch[0]
     * is information about the root; further values are for deeper subtrees being
     * explored.
     *
     * For every right branch taken to reach the position we're currently
     * working in, there will be a (non-nullopt) entry in m_branch corresponding
     * to the left branch at that level.
     *
     * For example, imagine this tree:     - N0 -
     *                                    /      \
     *                                   N1      N2
     *                                  /  \    /  \
     *                                 A    B  C   N3
     *                                            /  \
     *                                           D    E
     *
     * Initially, m_branch is empty. After processing leaf A, it would become
     * {nullopt, nullopt, A}. When processing leaf B, an entry at level 2 already
     * exists, and it would thus be combined with it to produce a level 1 one,
     * resulting in {nullopt, N1}. Adding C and D takes us to {nullopt, N1, C}
     * and {nullopt, N1, C, D} respectively. When E is processed, it is combined
     * with D, and then C, and then N1, to produce the root, resulting in {N0}.
     *
     * This structure allows processing with just O(log n) overhead if the leaves
     * are computed on the fly.
     *
     * As an invariant, there can never be nullopt entries at the end. There can
     * also not be more than 128 entries (as that would mean more than 128 levels
     * in the tree). The depth of newly added entries will always be at least
     * equal to the current size of m_branch (otherwise it does not correspond
     * to a depth-first traversal of a tree). m_branch is only empty if no entries
     * have ever be processed. m_branch having length 1 corresponds to being done.
     */
    std::vector<std::optional<NodeInfo>> m_branch;

    XOnlyPubKey m_internal_key;  //!< The internal key, set when finalizing.
    XOnlyPubKey m_output_key;    //!< The output key, computed when finalizing.
    bool m_parity;               //!< The tweak parity, computed when finalizing.

    /** Combine information about a parent Merkle tree node from its child nodes. */
    static NodeInfo Combine(NodeInfo&& a, NodeInfo&& b);
    /** Insert information about a node at a certain depth, and propagate information up. */
    void Insert(NodeInfo&& node, int depth);

public:
    /** Add a new script at a certain depth in the tree. Add() operations must be called
     *  in depth-first traversal order of binary tree. If track is true, it will be included in
     *  the GetSpendData() output. */
    TaprootBuilder& Add(int depth, Span<const unsigned char> script, int leaf_version, bool track = true);
    /** Like Add(), but for a Merkle node with a given hash to the tree. */
    TaprootBuilder& AddOmitted(int depth, const uint256& hash);
    /** Finalize the construction. Can only be called when IsComplete() is true.
        internal_key.IsFullyValid() must be true. */
    TaprootBuilder& Finalize(const XOnlyPubKey& internal_key);

    /** Return true if so far all input was valid. */
    bool IsValid() const { return m_valid; }
    /** Return whether there were either no leaves, or the leaves form a Huffman tree. */
    bool IsComplete() const { return m_valid && (m_branch.size() == 0 || (m_branch.size() == 1 && m_branch[0].has_value())); }
    /** Compute scriptPubKey (after Finalize()). */
    WitnessV1Taproot GetOutput();
    /** Check if a list of depths is legal (will lead to IsComplete()). */
    static bool ValidDepths(const std::vector<int>& depths);
    /** Compute spending data (after Finalize()). */
    TaprootSpendData GetSpendData() const;
    /** Returns a vector of tuples representing the depth, leaf version, and script */
    std::vector<std::tuple<uint8_t, uint8_t, std::vector<unsigned char>>> GetTreeTuples() const;
    /** Returns true if there are any tapscripts */
    bool HasScripts() const { return !m_branch.empty(); }
};

/** Given a TaprootSpendData and the output key, reconstruct its script tree.
 *
 * If the output doesn't match the spenddata, or if the data in spenddata is incomplete,
 * std::nullopt is returned. Otherwise, a vector of (depth, script, leaf_ver) tuples is
 * returned, corresponding to a depth-first traversal of the script tree.
 */
std::optional<std::vector<std::tuple<int, std::vector<unsigned char>, int>>> InferTaprootTree(const TaprootSpendData& spenddata, const XOnlyPubKey& output);

/** An interface to be implemented by keystores that support signing. */
class SigningProvider
{
public:
    virtual ~SigningProvider() = default;
    virtual bool GetCScript(const CScriptID &scriptid, CScript& script) const { return false; }
    virtual bool HaveCScript(const CScriptID &scriptid) const { return false; }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& pubkey) const { return false; }
    virtual bool GetKey(const CKeyID &address, CKey& key) const { return false; }
    virtual bool HaveKey(const CKeyID &address) const { return false; }
    virtual bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const { return false; }
    virtual bool GetHybridKey(const CKeyID &address, pqc::HybridKey& key) const { return false; }
    virtual bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const { return false; }
    virtual bool GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const { return false; }

    bool GetKeyByXOnly(const XOnlyPubKey& pubkey, CKey& key) const
    {
        for (const auto& id : pubkey.GetKeyIDs()) {
            if (GetKey(id, key)) return true;
        }
        return false;
    }

    bool GetPubKeyByXOnly(const XOnlyPubKey& pubkey, CPubKey& out) const
    {
        for (const auto& id : pubkey.GetKeyIDs()) {
            if (GetPubKey(id, out)) return true;
        }
        return false;
    }

    bool GetKeyOriginByXOnly(const XOnlyPubKey& pubkey, KeyOriginInfo& info) const
    {
        for (const auto& id : pubkey.GetKeyIDs()) {
            if (GetKeyOrigin(id, info)) return true;
        }
        return false;
    }
};

extern const SigningProvider& DUMMY_SIGNING_PROVIDER;

class HidingSigningProvider : public SigningProvider
{
private:
    const bool m_hide_secret;
    const bool m_hide_origin;
    const SigningProvider* m_provider;

public:
    HidingSigningProvider(const SigningProvider* provider, bool hide_secret, bool hide_origin) : m_hide_secret(hide_secret), m_hide_origin(hide_origin), m_provider(provider) {}
    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const override;
    bool GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const override;
};

struct FlatSigningProvider final : public SigningProvider
{
    std::map<CScriptID, CScript> scripts;
    std::map<CKeyID, CPubKey> pubkeys;
    std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> origins;
    std::map<CKeyID, CKey> keys;
    std::map<XOnlyPubKey, TaprootBuilder> tr_trees; /** Map from output key to Taproot tree (which can then make the TaprootSpendData */

    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const override;
    bool GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const override;

    FlatSigningProvider& Merge(FlatSigningProvider&& b) LIFETIMEBOUND;
};

/** Fillable signing provider that keeps keys in an address->secret map */
class FillableSigningProvider : public SigningProvider
{
protected:
    using KeyMap = std::map<CKeyID, CKey>;
    using ScriptMap = std::map<CScriptID, CScript>;
    using HybridKeyMap = std::map<CKeyID, pqc::HybridKey>;

    KeyMap mapKeys GUARDED_BY(cs_KeyStore);
    ScriptMap mapScripts GUARDED_BY(cs_KeyStore);
    HybridKeyMap mapHybridKeys GUARDED_BY(cs_KeyStore);

    void ImplicitlyLearnRelatedKeyScripts(const CPubKey& pubkey) EXCLUSIVE_LOCKS_REQUIRED(cs_KeyStore);

public:
    mutable RecursiveMutex cs_KeyStore;

    virtual bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    virtual bool AddKey(const CKey &key) { return AddKeyPubKey(key, key.GetPubKey()); }
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const override;
    virtual bool HaveKey(const CKeyID &address) const override;
    virtual std::set<CKeyID> GetKeys() const;
    virtual bool GetKey(const CKeyID &address, CKey &keyOut) const override;
    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const CScriptID &hash) const override;
    virtual std::set<CScriptID> GetCScripts() const;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const override;

    virtual bool AddHybridKey(const CKeyID& keyid, const pqc::HybridKey& hybrid_key)
    {
        LOCK(cs_KeyStore);
        mapHybridKeys[keyid] = hybrid_key;
        return true;
    }

    virtual bool GetHybridKey(const CKeyID& address, pqc::HybridKey& key) const override
    {
        LOCK(cs_KeyStore);
        HybridKeyMap::const_iterator mi = mapHybridKeys.find(address);
        if (mi != mapHybridKeys.end()) {
            key = mi->second;
            return true;
        }
        return false;
    }
};

/** Return the CKeyID of the key involved in a script (if there is a unique one). */
CKeyID GetKeyForDestination(const SigningProvider& store, const CTxDestination& dest);

/** A signing provider to be used to interface with multiple signing providers at once. */
class MultiSigningProvider final : public SigningProvider
{
private:
    std::vector<std::unique_ptr<SigningProvider>> m_providers;

public:
    void AddProvider(std::unique_ptr<SigningProvider> provider);

    bool GetCScript(const CScriptID& scriptid, CScript& script) const override;
    bool GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const override;
    bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override;
    bool GetKey(const CKeyID& keyid, CKey& key) const override;
    bool GetTaprootSpendData(const XOnlyPubKey& output_key, TaprootSpendData& spenddata) const override;
    bool GetTaprootBuilder(const XOnlyPubKey& output_key, TaprootBuilder& builder) const override;
    bool GetHybridKey(const CKeyID& keyid, pqc::HybridKey& key) const override
    {
        for (const auto& provider : m_providers) {
            if (provider->GetHybridKey(keyid, key)) {
                return true;
            }
        }
        return false;
    }
};

#endif // BITCOIN_SCRIPT_SIGNINGPROVIDER_H
