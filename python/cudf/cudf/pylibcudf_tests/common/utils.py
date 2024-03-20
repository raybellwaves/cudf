# Copyright (c) 2024, NVIDIA CORPORATION.

import pyarrow as pa

from cudf._lib import pylibcudf as plc


def assert_array_eq(plc_column, pa_array):
    """Verify that the pylibcudf array and PyArrow array are equal."""
    pa_equal = pa.compute.equal(plc.interop.to_arrow(plc_column), pa_array)
    assert pa.compute.all(pa_equal).as_py()


def assert_table_eq(plc_table, pa_table):
    """Verify that the pylibcudf array and PyArrow array are equal."""
    plc_shape = (plc_table.num_rows(), plc_table.num_columns())
    assert plc_shape == pa_table.shape

    for plc_col, pa_col in zip(plc_table.columns(), pa_table.columns):
        assert_array_eq(plc_col, pa_col)