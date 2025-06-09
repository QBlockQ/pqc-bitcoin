/**
 * Copyright (c) 2025 QBlock & Qbits
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 * 
 * @file pow_q.h
 * @brief Quantum-resistant proof-of-work declarations
 */

#ifndef BITCOIN_POW_Q_H
#define BITCOIN_POW_Q_H

#include <consensus/params.h>

class CBlockIndex;
class CBlock;
class uint256;

/**
 * Check if a block hash satisfies the quantum-resistant proof-of-work requirement
 * 
 * @param hash The block hash to check
 * @param nBits The target difficulty
 * @param params Consensus parameters
 * @return true if the hash satisfies the quantum-resistant PoW requirement
 */
bool CheckQuantumProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params);

/**
 * Perform quantum-resistant proof-of-work on a block
 * 
 * @param pblock Block to mine
 * @param nBits Target difficulty
 * @param params Consensus parameters
 * @return true if successful, false otherwise
 */
bool PerformQuantumProofOfWork(CBlock* pblock, uint32_t nBits, const Consensus::Params& params);

/**
 * Calculate the quantum-resistant difficulty adjustment
 * 
 * @param pindexLast Last block in the chain
 * @param params Consensus parameters
 * @return New bits value for next block
 */
unsigned int GetNextQuantumWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params);

#endif // BITCOIN_POW_Q_H
