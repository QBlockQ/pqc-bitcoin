/**
 * Copyright (c) 2025 QBlock & Qbits
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 * 
 * @file falcon_agg.h
 * @brief Aggregation functionality for Falcon post-quantum signatures
 */

#ifndef BITCOIN_CRYPTO_FALCONAGG_H
#define BITCOIN_CRYPTO_FALCONAGG_H

#include <vector>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace falcon_agg {

/**
 * Falcon signature structure containing both the signature and public key
 */
struct FalconSignature {
    std::vector<uint8_t> sig;
    std::vector<uint8_t> pubkey;
    
    /** Empty constructor */
    FalconSignature() {}
    
    /** Constructor with signature and public key data */
    FalconSignature(const std::vector<uint8_t>& signature, const std::vector<uint8_t>& public_key)
        : sig(signature), pubkey(public_key) {}
};

/**
 * Structure to hold an aggregated Falcon signature along with the public keys it corresponds to
 */
struct AggregatedFalconSig {
    std::vector<uint8_t> aggregated_sig;
    std::vector<std::vector<uint8_t>> pubkeys;
    size_t num_signatures;
    
    /** Empty constructor */
    AggregatedFalconSig() : num_signatures(0) {}
};

/**
 * Aggregate multiple Falcon signatures into a single aggregated signature
 * 
 * @param signatures Vector of individual Falcon signatures to aggregate
 * @param out_aggregated Output parameter for the aggregated signature
 * @return true if aggregation was successful, false otherwise
 */
bool AggregateFalconSignatures(
    const std::vector<FalconSignature>& signatures,
    AggregatedFalconSig& out_aggregated);

/**
 * Verify an aggregated Falcon signature against a message and the corresponding public keys
 * 
 * @param agg_sig The aggregated signature to verify
 * @param message The message that was signed
 * @return true if the signature is valid, false otherwise
 */
bool VerifyAggregatedFalconSig(
    const AggregatedFalconSig& agg_sig,
    const std::vector<uint8_t>& message);

/**
 * Calculate the size of an aggregated signature
 * 
 * @param num_signatures Number of signatures that were aggregated
 * @return Size in bytes of the aggregated signature
 */
size_t GetAggregatedSigSize(size_t num_signatures);

/**
 * Calculate the compression ratio achieved by aggregation
 * 
 * @param original_size Total size of all individual signatures
 * @param compressed_size Size of the aggregated signature
 * @return Compression ratio (original_size / compressed_size)
 */
double GetCompressionRatio(size_t original_size, size_t compressed_size);

} // namespace falcon_agg

#endif // BITCOIN_CRYPTO_FALCONAGG_H
