/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <benchmarks/common/generate_input.hpp>
#include <benchmarks/fixture/benchmark_fixture.hpp>
#include <benchmarks/io/cuio_common.hpp>
#include <benchmarks/io/nvbench_helpers.hpp>

#include <cudf/io/orc.hpp>
#include <cudf/utilities/default_stream.hpp>

#include <nvbench/nvbench.cuh>

namespace {

// Size of the data in the benchmark dataframe; chosen to be low enough to allow benchmarks to
// run on most GPUs, but large enough to allow highest throughput
constexpr int64_t data_size        = 512 << 20;
constexpr cudf::size_type num_cols = 64;

template <typename Timer>
void read_once(cudf::io::orc_reader_options const& options,
               cudf::size_type num_rows_to_read,
               Timer& timer)
{
  timer.start();
  auto const result = cudf::io::read_orc(options);
  timer.stop();

  CUDF_EXPECTS(result.tbl->num_columns() == num_cols, "Unexpected number of columns");
  CUDF_EXPECTS(result.tbl->num_rows() == num_rows_to_read, "Unexpected number of rows");
}

template <typename Timer>
void chunked_read(cudf::io::orc_reader_options const& options,
                  cudf::size_type num_rows_to_read,
                  cudf::size_type appox_num_chunks,
                  Timer& timer)
{
  // Create a chunked reader that has an internal memory limits to process around 10 chunks.
  auto const output_limit = static_cast<std::size_t>(data_size / appox_num_chunks);
  auto const input_limit  = output_limit * 10;

  auto reader = cudf::io::chunked_orc_reader(output_limit, input_limit, options);
  cudf::size_type num_rows{0};

  timer.start();
  do {
    auto chunk = reader.read_chunk();
    num_rows += chunk.tbl->num_rows();
  } while (reader.has_next());
  timer.stop();

  CUDF_EXPECTS(num_rows == num_rows_to_read, "Unexpected number of rows");
}

template <bool is_chunked_read>
void orc_read_common(cudf::size_type num_rows_to_read,
                     cuio_source_sink_pair& source_sink,
                     nvbench::state& state)
{
  auto const read_opts =
    cudf::io::orc_reader_options::builder(source_sink.make_source_info()).build();
  cudf::size_type constexpr approx_num_chunks = 10;

  auto mem_stats_logger = cudf::memory_stats_logger();  // init stats logger
  state.set_cuda_stream(nvbench::make_cuda_stream_view(cudf::get_default_stream().value()));
  state.exec(nvbench::exec_tag::sync | nvbench::exec_tag::timer,
             [&](nvbench::launch&, auto& timer) {
               try_drop_l3_cache();

               if constexpr (!is_chunked_read) {
                 read_once(read_opts, num_rows_to_read, timer);
               } else {
                 chunked_read(read_opts, num_rows_to_read, approx_num_chunks, timer);
               }
             });

  auto const time = state.get_summary("nv/cold/time/gpu/mean").get_float64("value");
  state.add_element_count(static_cast<double>(data_size) / time, "bytes_per_second");
  state.add_buffer_size(
    mem_stats_logger.peak_memory_usage(), "peak_memory_usage", "peak_memory_usage");
  state.add_buffer_size(source_sink.size(), "encoded_file_size", "encoded_file_size");
}

}  // namespace

template <data_type DataType, cudf::io::io_type IOType>
void BM_orc_read_data(nvbench::state& state,
                      nvbench::type_list<nvbench::enum_type<DataType>, nvbench::enum_type<IOType>>)
{
  auto const d_type                 = get_type_or_group(static_cast<int32_t>(DataType));
  cudf::size_type const cardinality = state.get_int64("cardinality");
  cudf::size_type const run_length  = state.get_int64("run_length");
  cuio_source_sink_pair source_sink(IOType);

  auto const num_rows_written = [&]() {
    auto const tbl = create_random_table(
      cycle_dtypes(d_type, num_cols),
      table_size_bytes{data_size},
      data_profile_builder().cardinality(cardinality).avg_run_length(run_length));
    auto const view = tbl->view();

    cudf::io::orc_writer_options opts =
      cudf::io::orc_writer_options::builder(source_sink.make_sink_info(), view);
    cudf::io::write_orc(opts);
    return view.num_rows();
  }();

  orc_read_common<false>(num_rows_written, source_sink, state);
}

template <cudf::io::io_type IOType, cudf::io::compression_type Compression>
void BM_orc_read_io_compression(
  nvbench::state& state,
  nvbench::type_list<nvbench::enum_type<IOType>, nvbench::enum_type<Compression>>)
{
  auto const d_type = get_type_or_group({static_cast<int32_t>(data_type::INTEGRAL_SIGNED),
                                         static_cast<int32_t>(data_type::FLOAT),
                                         static_cast<int32_t>(data_type::DECIMAL),
                                         static_cast<int32_t>(data_type::TIMESTAMP),
                                         static_cast<int32_t>(data_type::STRING),
                                         static_cast<int32_t>(data_type::LIST),
                                         static_cast<int32_t>(data_type::STRUCT)});

  cudf::size_type const cardinality = state.get_int64("cardinality");
  cudf::size_type const run_length  = state.get_int64("run_length");
  cuio_source_sink_pair source_sink(IOType);

  auto const num_rows_written = [&]() {
    auto const tbl = create_random_table(
      cycle_dtypes(d_type, num_cols),
      table_size_bytes{data_size},
      data_profile_builder().cardinality(cardinality).avg_run_length(run_length));
    auto const view = tbl->view();

    cudf::io::orc_writer_options opts =
      cudf::io::orc_writer_options::builder(source_sink.make_sink_info(), view)
        .compression(Compression);
    cudf::io::write_orc(opts);
    return view.num_rows();
  }();

  auto const is_chunked_read = static_cast<bool>(state.get_int64("chunked_read"));
  if (is_chunked_read) {
    orc_read_common<true>(num_rows_written, source_sink, state);
  } else {
    orc_read_common<false>(num_rows_written, source_sink, state);
  }
}

using d_type_list = nvbench::enum_type_list<data_type::INTEGRAL_SIGNED,
                                            data_type::FLOAT,
                                            data_type::DECIMAL,
                                            data_type::TIMESTAMP,
                                            data_type::STRING,
                                            data_type::LIST,
                                            data_type::STRUCT>;

using io_list = nvbench::enum_type_list<cudf::io::io_type::FILEPATH,
                                        cudf::io::io_type::HOST_BUFFER,
                                        cudf::io::io_type::DEVICE_BUFFER>;

using compression_list =
  nvbench::enum_type_list<cudf::io::compression_type::SNAPPY, cudf::io::compression_type::NONE>;

NVBENCH_BENCH_TYPES(BM_orc_read_data,
                    NVBENCH_TYPE_AXES(d_type_list,
                                      nvbench::enum_type_list<cudf::io::io_type::DEVICE_BUFFER>))
  .set_name("orc_read_decode")
  .set_type_axes_names({"data_type", "io"})
  .set_min_samples(4)
  .add_int64_axis("cardinality", {0, 1000})
  .add_int64_axis("run_length", {1, 32});

NVBENCH_BENCH_TYPES(BM_orc_read_io_compression, NVBENCH_TYPE_AXES(io_list, compression_list))
  .set_name("orc_read_io_compression")
  .set_type_axes_names({"io", "compression"})
  .set_min_samples(4)
  .add_int64_axis("cardinality", {0, 1000})
  .add_int64_axis("run_length", {1, 32})
  .add_int64_axis("chunked_read", {0, 1});
