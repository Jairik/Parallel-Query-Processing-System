# Data Schema & Synthesization Plans

## Attributes

| Name | Type | Description |
|---|---|---|
| command_id | unsigned long long | Unique ID for each tuple |
| raw_command | string | Full command string |
| base_command | string | Root utility invoked (e.g., `ls`, `grep`, `docker`) |
| shell_type | string | Shell used (e.g., `bash`, `zsh`) |
| exit_code | int | Command's return code |
| timestamp | string | When the command was executed (standardized, e.g., ms) |
| sudo_used | bool | Whether the command was run with `sudo` |
| working_directory | string | Path from where the command was executed |
| user_id | int | User ID of the user who executed the command |
| user_name | string | Username of the user who executed the command |
| host_name | string | System hostname or identifier |
| risk_level | int / enum | Heuristic risk score (e.g., `1` = safe, `5` = destructive) |

12 total attributes, which will allow for sufficient querying.

## Best Indexes

There should be minimal default row indexes, as each one will require a seperate B+ tree to be stored in memory. Below are the currently chosen *default* indexes:

- TODO

## Generation

Acheived via `generate_commands.py`. Will use some C-level ML to dynamically assign risk level at runtime at later phases.
