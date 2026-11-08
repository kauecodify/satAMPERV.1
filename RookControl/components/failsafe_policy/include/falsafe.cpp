                 ┌──────────────┐
   init ────────►│   ENV_OK     │◄──── temp < warn-hyst
                 └──────┬───────┘
                        │ temp ≥ warn (por ≥ min_dwell)
                        ▼
                 ┌──────────────┐
                 │  ENV_WARN    │──────► log + flag no payload
                 └──────┬───────┘
                        │ temp ≥ critical (por ≥ min_dwell)
                        ▼
                 ┌──────────────┐
                 │ ENV_CRITICAL │──┬── fc_healthy && !fc_in_failsafe
                 └──────┬───────┘  │       └─► retorna FS_ACTION_RTL
                        │          └── caso contrário: FS_ACTION_LOG
      stale > stale_ms  │
                        ▼
                 ┌──────────────┐
                 │  ENV_STALE   │──────► FS_ACTION_LOG (sem RTL)
                 └──────────────┘
