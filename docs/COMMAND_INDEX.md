# Customer Command Index

This Customer repo includes a delivery-safe copy of the Product command helper.

## Entrypoints

| Need | Entrypoint |
| --- | --- |
| Customer quick commands | `docs/OPERATOR_QUICK_COMMANDS.md` |
| Helper script | `tools/uwb_ops.ps1` |
| Provision script | `tools/provision.ps1` |
| Delivery hash check | `powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 hash-delivery -Repo customer` |

## Common Commands

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 -Help
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 status -Repo customer
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 hash-delivery -Repo customer
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 list-stlink
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 flash -Repo customer -Role anc1 -Mode ds -Sn <SN>
powershell -ExecutionPolicy Bypass -File .\tools\uwb_ops.ps1 capture-log -Repo customer -Port COM18 -Name anchor1 -DurationSec 60
```

## Policy

Product is the canonical source for command kit behavior. Customer carries this helper so field validation can flash delivery firmware, check hashes, inspect status, list ST-LINK probes, and capture UART logs without depending on a Product checkout shell.

Do not hand-edit Customer runtime code. Product fixes must arrive through Product clean export.
