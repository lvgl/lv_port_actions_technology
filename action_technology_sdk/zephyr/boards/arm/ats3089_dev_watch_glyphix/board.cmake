# SPDX-License-Identifier: Apache-2.0

# pyocd config
board_runner_args(pyocd "--target=leopard")
board_set_flasher_ifnset(pyocd)
board_set_debugger_ifnset(pyocd)
board_finalize_runner_args(pyocd)

# jlink config
board_runner_args(jlink "--device=leopard" "--speed=10000")
board_set_flasher_ifnset(jlink)
board_set_debugger_ifnset(jlink)
board_finalize_runner_args(jlink)

