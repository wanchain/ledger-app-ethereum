#include "shared_context.h"
#include "apdu_constants.h"
#include "ui_flow.h"
#include "tokens.h"

static void verify_signature(uint8_t *hash, uint8_t *signature, size_t signature_size) {
    cx_ecfp_public_key_t tokenKey;

    cx_ecfp_init_public_key(CX_CURVE_256K1,
                            LEDGER_SIGNATURE_PUBLIC_KEY,
                            sizeof(LEDGER_SIGNATURE_PUBLIC_KEY),
                            &tokenKey);
    if (!cx_ecdsa_verify(&tokenKey,
                         CX_LAST,
                         CX_SHA256,
                         hash,
                         32,
                         signature,
                         signature_size)) {
#ifndef HAVE_BYPASS_SIGNATURES
        PRINTF("Invalid token signature\n");
        THROW(0x6A80);
#endif
    }
}

#ifdef HAVE_CONTRACT_NAME_IN_DESCRIPTOR

void handleProvideErc20TokenInformation(uint8_t p1,
                                        uint8_t p2,
                                        uint8_t *workBuffer,
                                        uint16_t dataLength,
                                        unsigned int *flags,
                                        unsigned int *tx) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(flags);
    uint32_t offset = 0;
    uint8_t tickerLength, contractNameLength;
    uint32_t chainId;
    uint8_t hash[INT256_LENGTH];
    cx_sha256_t sha256;

    cx_sha256_init(&sha256);

    tmpCtx.transactionContext.currentItemIndex =
        (tmpCtx.transactionContext.currentItemIndex + 1) % MAX_ITEMS;
    tokenDefinition_t *token =
        &tmpCtx.transactionContext.tokens[tmpCtx.transactionContext.currentItemIndex];

    if (dataLength < 1) {
        THROW(0x6A80);
    }
    tickerLength = workBuffer[offset++];
    dataLength--;
    if ((tickerLength + 2) >= sizeof(token->ticker)) {  // +2 because ' \0' is appended to ticker
        THROW(0x6A80);
    }
    if (dataLength < tickerLength + 1) {
        THROW(0x6A80);
    }
    cx_hash((cx_hash_t *) &sha256, 0, workBuffer + offset, tickerLength, NULL, 0);
    memmove(token->ticker, workBuffer + offset, tickerLength);
    token->ticker[tickerLength] = ' ';
    token->ticker[tickerLength + 1] = '\0';
    offset += tickerLength;
    dataLength -= tickerLength;

    contractNameLength = workBuffer[offset++];
    dataLength--;
    if (dataLength < contractNameLength + 20 + 4 + 4) {
        THROW(0x6A80);
    }
    cx_hash((cx_hash_t *) &sha256,
            CX_LAST,
            workBuffer + offset,
            contractNameLength + 20 + 4 + 4,
            hash,
            32);
    memmove(token->contractName,
            workBuffer + offset,
            MIN(contractNameLength, sizeof(token->contractName) - 1));
    token->contractName[MIN(contractNameLength, sizeof(token->contractName) - 1)] = '\0';
    offset += contractNameLength;
    dataLength -= contractNameLength;

    memmove(token->address, workBuffer + offset, 20);
    offset += 20;
    dataLength -= 20;
    token->decimals = U4BE(workBuffer, offset);
    offset += 4;
    dataLength -= 4;
    chainId = U4BE(workBuffer, offset);
    if ((chainConfig->chainId != 0) && (chainConfig->chainId != chainId)) {
        PRINTF("ChainId token mismatch\n");
        THROW(0x6A80);
    }
    offset += 4;
    dataLength -= 4;

    verify_signature(hash, workBuffer + offset, dataLength);

    tmpCtx.transactionContext.tokenSet[tmpCtx.transactionContext.currentItemIndex] = 1;
    THROW(0x9000);
}

#else

static tokenDefinition_t *whitelisted_descriptor(tokenDefinition_t *token) {
#ifdef HAVE_TOKENS_EXTRA_LIST
    uint32_t index;
    for (index = 0; index < NUM_TOKENS_EXTRA; index++) {
        tokenDefinition_t *currentToken = (tokenDefinition_t *) PIC(&TOKENS_EXTRA[index]);
        if (memcmp(currentToken->address, token->address, 20) == 0) {
            PRINTF("Descriptor whitelisted\n");
            return currentToken;
        }
    }
#else
    UNUSED(token);
#endif

    return NULL;
}

void handleProvideErc20TokenInformation(uint8_t p1,
                                        uint8_t p2,
                                        uint8_t *workBuffer,
                                        uint16_t dataLength,
                                        unsigned int *flags,
                                        __attribute__((unused)) unsigned int *tx) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(flags);
    uint32_t offset = 0;
    uint8_t tickerLength;
    uint32_t chainId;
    uint8_t hash[INT256_LENGTH];

    tmpCtx.transactionContext.currentItemIndex =
        (tmpCtx.transactionContext.currentItemIndex + 1) % MAX_ITEMS;
    tokenDefinition_t *token =
        &tmpCtx.transactionContext.extraInfo[tmpCtx.transactionContext.currentItemIndex].token;

    PRINTF("Provisioning currentItemIndex %d\n", tmpCtx.transactionContext.currentItemIndex);

    if (dataLength < 1) {
        THROW(0x6A80);
    }
    tickerLength = workBuffer[offset++];
    dataLength--;
    if ((tickerLength + 1) >= sizeof(token->ticker)) {
        THROW(0x6A80);
    }
    if (dataLength < tickerLength + 20 + 4 + 4) {
        THROW(0x6A80);
    }
    cx_hash_sha256(workBuffer + offset, tickerLength + 20 + 4 + 4, hash, 32);
    memmove(token->ticker, workBuffer + offset, tickerLength);
    token->ticker[tickerLength] = ' ';
    token->ticker[tickerLength + 1] = '\0';
    offset += tickerLength;
    dataLength -= tickerLength;
    memmove(token->address, workBuffer + offset, 20);
    offset += 20;
    dataLength -= 20;
    token->decimals = U4BE(workBuffer, offset);
    offset += 4;
    dataLength -= 4;
    chainId = U4BE(workBuffer, offset);
    if ((chainConfig->chainId != ETHEREUM_MAINNET_CHAINID) && (chainConfig->chainId != chainId)) {
        PRINTF("ChainId token mismatch: %d vs %d\n", chainConfig->chainId, chainId);
        THROW(0x6A80);
    }
    offset += 4;
    dataLength -= 4;

    tokenDefinition_t *whiteListedToken = whitelisted_descriptor(token);
    if (whiteListedToken == NULL) {
        verify_signature(hash, workBuffer + offset, dataLength);
    } else {
        strcpy((char *) token->ticker, (char *) whiteListedToken->ticker);
        token->decimals = whiteListedToken->decimals;
    }

    tmpCtx.transactionContext.tokenSet[tmpCtx.transactionContext.currentItemIndex] = 1;
    THROW(0x9000);
}

#endif
