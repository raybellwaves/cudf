# <div align="left"><img src="img/rapids_logo.png" width="90px"/>&nbsp;cuDF - GPU DataFrames</div>

<figure>
<img src="docs/cudf/source/_static/colab.png" width="200" alt="colab" />
<figcaption style="text-align: center;">Try it on Google Colab!</figcaption>
</figure>

cuDF is a GPU DataFrame library for loading joining, aggregating,
filtering, and otherwise manipulating data.

You can use cuDF directly, or easily accelerate existing pandas
using `cudf.pandas`:

<table>
<thead>
<tr class="header">
<th>Using cuDF directly</th>
<th>Using cudf.pandas</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td><pre lang="python">
<code>
import cudf
import requests
from io import StringIO
&#10;
url = &quot;https://github.com/plotly/datasets/raw/master/tips.csv&quot;
content = requests.get(url).content.decode(&#39;utf-8&#39;)
&#10;tips_df = cudf.read_csv(StringIO(content))
tips_df[&#39;tip_percentage&#39;] = tips_df[&#39;tip&#39;] / tips_df[&#39;total_bill&#39;] * 100
&#10;# display average tip by dining party size
print(tips_df.groupby(&#39;size&#39;).tip_percentage.mean())
</code></pre></td>
<td><pre lang="python">
<code>
%load_ext cudf.pandas  # pandas operations use the GPU!

import pandas as pd
import requests
from io import StringIO
&#10;
url = &quot;https://github.com/plotly/datasets/raw/master/tips.csv&quot;
content = requests.get(url).content.decode(&#39;utf-8&#39;)
&#10;tips_df = pd.read_csv(StringIO(content))
tips_df[&#39;tip_percentage&#39;] = tips_df[&#39;tip&#39;] / tips_df[&#39;total_bill&#39;] * 100
&#10;# display average tip by dining party size
print(tips_df.groupby(&#39;size&#39;).tip_percentage.mean())
</tr>
</tbody>
</table>

```
size
1    21.729201548727808
2    16.571919173482897
3    15.215685473711837
4    14.594900639351332
5    14.149548965142023
6    15.622920072028379
Name: tip_percentage, dtype: float64
```

- [Install](https://rapids.ai/start.html): Instructions for installing cuDF and other [RAPIDS](https://rapids.ai) libraries.
- [Python documentation](https://docs.rapids.ai/api/cudf/stable/)Python API reference, tutorials, and topic guides.
- [libcudf (C++/CUDA) documentation](https://docs.rapids.ai/api/libcudf/stable/)
- [RAPIDS Community](https://rapids.ai/community.html): Get help, contribute, and collaborate.

## Installation

### CUDA/GPU requirements

* CUDA 11.2+
* NVIDIA driver 450.80.02+
* Pascal architecture or better (Compute Capability >=6.0)

### Conda

cuDF can be installed with conda (via [miniconda](https://conda.io/miniconda.html) or the full [Anaconda distribution](https://www.anaconda.com/download)) from the `rapidsai` channel:

```bash
conda install -c rapidsai -c conda-forge -c nvidia \
    cudf=23.10 python=3.10 cuda-version=11.8
```

We also provide [nightly Conda packages](https://anaconda.org/rapidsai-nightly) built from the HEAD
of our latest development branch.

Note: cuDF is supported only on Linux, and with Python versions 3.9 and later.

See the [Get RAPIDS version picker](https://rapids.ai/start.html) for more OS and version info.

## Build/Install from Source
See build [instructions](CONTRIBUTING.md#setting-up-your-build-environment).

## Contributing

Please see our [guide for contributing to cuDF](CONTRIBUTING.md).

## Contact

Find out more details on the [RAPIDS site](https://rapids.ai/community.html)

## <div align="left"><img src="img/rapids_logo.png" width="265px"/></div> Open GPU Data Science

The RAPIDS suite of open source software libraries aim to enable execution of end-to-end data science and analytics pipelines entirely on GPUs. It relies on NVIDIA® CUDA® primitives for low-level compute optimization, but exposing that GPU parallelism and high-bandwidth memory speed through user-friendly Python interfaces.

<p align="center"><img src="img/rapids_arrow.png" width="80%"/></p>

### Apache Arrow on GPU

The GPU version of [Apache Arrow](https://arrow.apache.org/) is a common API that enables efficient interchange of tabular data between processes running on the GPU. End-to-end computation on the GPU avoids unnecessary copying and converting of data off the GPU, reducing compute time and cost for high-performance analytics common in artificial intelligence workloads. As the name implies, cuDF uses the Apache Arrow columnar data format on the GPU. Currently, a subset of the features in Apache Arrow are supported.
