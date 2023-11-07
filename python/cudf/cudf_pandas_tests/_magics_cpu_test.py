# SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
# All rights reserved.
# SPDX-License-Identifier: LicenseRef-NvidiaProprietary
#
# NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
# property and proprietary rights in and to this material, related
# documentation and any modifications thereto. Any use, reproduction,
# disclosure or distribution of this material and related documentation
# without an express license agreement from NVIDIA CORPORATION or
# its affiliates is strictly prohibited.


def ipython_magics_cpu_test():
    import warnings

    from IPython.core.interactiveshell import InteractiveShell

    # Use in-memory history file to avoid file handles leaking
    # https://github.com/pandas-dev/pandas/pull/35711
    from traitlets.config import Config  # isort:skip

    warnings.filterwarnings("ignore", category=UserWarning)

    c = Config()
    c.HistoryManager.hist_file = ":memory:"

    ip = InteractiveShell(config=c)
    ip.run_line_magic("load_ext", "cudf.pandas")

    # confirm pandas is not aliased
    ip.run_cell("import pandas as pd; s = pd.Series(range(5))")
    result = ip.run_cell("assert not hasattr(s, '_fsproxy_state')")
    result.raise_error()


if __name__ == "__main__":
    ipython_magics_cpu_test()