// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

// Test LegacyScriptPubKeyMan::CanProvide behavior, making sure it returns true
// for recognized scripts even when keys may not be available for signing.
BOOST_AUTO_TEST_CASE(CanProvide)
{
    // Set up wallet and keyman variables.
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LegacyScriptPubKeyMan& keyman = *wallet.GetOrCreateLegacyScriptPubKeyMan();

    // Make a 1 of 2 multisig script
    std::vector<CKey> keys(2);
    std::vector<CPubKey> pubkeys;
    for (CKey& key : keys) {
        key.MakeNewKey(true);
        pubkeys.emplace_back(key.GetPubKey());
    }
    CScript multisig_script = GetScriptForMultisig(1, pubkeys);
    CScript p2sh_script = GetScriptForDestination(ScriptHash(multisig_script));
    SignatureData data;

    // Verify the p2sh(multisig) script is not recognized until the multisig
    // script is added to the keystore to make it solvable
    BOOST_CHECK(!keyman.CanProvide(p2sh_script, data));
    keyman.AddCScript(multisig_script);
    BOOST_CHECK(keyman.CanProvide(p2sh_script, data));
}

// Minimal smoke test for LegacyScriptPubKeyMan stubbed helpers used for macOS compatibility.
BOOST_AUTO_TEST_CASE(LegacyStubSmokeTest)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LegacyScriptPubKeyMan& keyman = *wallet.GetOrCreateLegacyScriptPubKeyMan();

    // NewKeyPool should be a no-op that succeeds for smoke-tests.
    BOOST_CHECK(keyman.NewKeyPool());

    // GenerateNewSeed should return a valid public key.
    CPubKey seed_pub = keyman.GenerateNewSeed();
    BOOST_CHECK(seed_pub.IsValid());
}

// Minimal smoke test for DescriptorScriptPubKeyMan stubbed helpers.
BOOST_AUTO_TEST_CASE(DescriptorStubSmokeTest)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());

    // Generate a minimal key and descriptor string.
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    std::string desc_str = "pkh(" + HexStr(pubkey) + ")";

    FlatSigningProvider keys;
    std::string error;
    std::unique_ptr<Descriptor> desc = Parse(desc_str, keys, error, /*require_checksum=*/false);
    BOOST_CHECK(desc);

    WalletDescriptor w_desc(std::move(desc), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/0, /*next_index=*/0);
    DescriptorScriptPubKeyMan desc_spkman(wallet, w_desc, /*keypool_size=*/0);

    CExtKey dummy_master;
    WalletBatch batch(wallet.GetDatabase());
    BOOST_CHECK(!desc_spkman.SetupDescriptorGeneration(batch, dummy_master, OutputType::LEGACY, /*internal=*/false));
    BOOST_CHECK(!desc_spkman.Encrypt(CKeyingMaterial(), nullptr));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
