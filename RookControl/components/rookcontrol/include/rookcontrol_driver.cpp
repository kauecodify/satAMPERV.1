struct RookCtx {
    int         port;
    uint8_t     addr;
    bool        bus_ok;
    uint32_t    last_ok_ms;
    int16_t     history[3];      // mediana de 3
    uint8_t     hist_idx;
    uint8_t     identical_cnt;   // sensor travado
    rook_data_t last;
};
static RookCtx s_ctxs[MAX_ROOK_SENSORS];  // normalmente 1
