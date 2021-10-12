# MASA-CUDAlign

<p align="justify">
The <b>MASA-CUDAlign extension</b> is used with the <a href="https://github.com/edanssandes/MASA-Core">MASA architecture</a> to align DNA sequences of unrestricted size with the Smith-Waterman and Needleman-Wunsch algorithms combined with Myers-Miller. It uses the NVIDIA CUDA platform to accelerate the computation time. This extension is able to align huge DNA sequences with more than 200 million base pairs (MBP). The current version is equivalent to CUDAlign 4.0 publication<sup>[TPDS2016](#references)</sup>, using multiple Fickett Band optimation in stage 4<sup>[CCPE2019](#references)</sup>.
</p>

### Download

Latest Version: [masa-cudalign-4.0.2.1028.tar.gz](releases/masa-cudalign-4.0.2.1028.tar.gz?raw=true)

### Compiling (cudalign)

```
tar -xvzf masa-cudalign-4.0.2.1028.tar.gz
cd masa-cudalign-4.0.2.1028
./configure
make
```

### Executing CUDAlign

```
./cudalign [options] seq1.fasta seq2.fasta
```
All the command line arguments can be retrieved using the --help parameter. See the [wiki](https://github.com/edanssandes/MASA-Core/wiki/Command-line-examples) for a list of command line examples.

### Performance Benchmarks

<p align="justify">
We have executed MASA-CUDAlign in many different environments. Here we will present the best results in different scenarios. We recommend to read the reference papers in order to understand the feature improvements in each test.
</p>

**Test Environment (Homogeneous GPU cluster)**: <a href="http://dx.doi.org/10.1109/CCGrid.2014.18"><font size=1>[CCGRID2014]</font></a><br>
Minotauro Cluster - 64 x NVidia Tesla M2090. 

Sequence 1 | Sequence 2 | Len1 | Len2 | Time | GCUPS
--- | --- | --- | --- | --- | --- |
NC_000001.10 | NC_006468.3 | 249M | 228M | 9h09m25s | **1726.47**

**Test Environment (Heterogeneous GPU Nodes)**: <a href="http://dx.doi.org/10.1145/2555243.2555280"><font size=1>[PPOPP2014]</font></a><br>
Laico Labs - 3 Hosts - 1 x NVidia GTX 580 + 2 x NVidia GTX 680. 

Sequence 1 | Sequence 2 | Len1 | Len2 | Time | GCUPS
--- | --- | --- | --- | --- | --- |
NC_000019.9 | NC_006486.3 | 59M | 64M | 7h29m17s | 139.60
NC_000020.10 | NC_006487.3 | 63M | 62M | 7h42m08s | 140.31
NC_000021.8 | NC_006488.2 | 48M | 46M | 4h27m04s | 139.63
NC_000022.10 | NC_006489.3 | 51M | 50M | 5h03m00s | **140.36**


**Test Environment (Heterogeneous GPUs in Single Node)**: <a href="http://dx.doi.org/10.1145/2555243.2555280"><font size=1>[PPOPP2014]</font></a><br>
Panoramix Host - 1 x NVidia Tesla K20c + 2 x NVidia Tesla C2050. 

Sequence 1 | Sequence 2 | Len1 | Len2 | Time | GCUPS
--- | --- | --- | --- | --- | --- |
NC_000019.9 | NC_006486.3 | 59M | 64M | 10h20m52s | 101.02
NC_000020.10 | NC_006487.3 | 63M | 62M | 7h42m08s | 100.96
NC_000021.8 | NC_006488.2 | 48M | 46M | 6h10m38s | 100.62
NC_000022.10 | NC_006489.3 | 51M | 50M | 7h03m36s | **101.38**

**Test Environment (Single GPU)**: <a href="http://dx.doi.org/10.1109/TPDS.2012.194"><font size=1>[TPDS2013]</font></a><br>
NVIDIA GeForce GTX 560 Ti 

Sequence 1 | Sequence 2 | Len1 | Len2 | Time | GCUPS
--- | --- | --- | --- | --- | --- |
CP000051.1 | CP000051.1 | 1M | 1M | 43s | 25.82
BA000035.2 | BX927147.1 | 3M | 3M | 6m07s | 28.15
AE016879.1 | AE017225.1 | 5M | 5M | 9m18s | 48.98
NC_005027.1 | NC_003997.3 | 7M | 5M | 22m01s | 28.28
NT_033779.4 | NT_037436.3 | 23M | 25M | 5h29m17s | 28.59
NC_000024.9 | NC_006492.2 | 59M | 24M | 13h05m23s | 30.18
BA000046.3 | NC_000021.7 | 33M | 47M | 8h26m09s | **50.70**






### License:

MASA-CUDAlign is an open source project with public license (GPLv3). A copy of the [LICENSE](https://raw.githubusercontent.com/edanssandes/MASA-CUDAlign/master/LICENSE) is maintained in this repository. 


### References:

<table border="0">
<tr>
<td><a href="https://doi.org/10.1089/cmb.2019.0031"><font size=1>[JCB2019]</font></a></td>
<td><sub>Using Multiple Fickett Bands to Accelerate Biological Sequence Comparisons. JCB 2019: 908-922. Gabriel Silva, Edans Sandes, George Teodoro, Alba Melo</sub></td>
</tr>
<tr>
<td><a href="http://dx.doi.org/10.1145/2858656"><font size=1>[TOPC2016]</font></a></td>
<td><sub>MASA: a Multi-Platform Architecture for Sequence Aligners with Block Pruning. TOPC:2(4):28. Edans Sandes, Guillermo Miranda, Xavier Martorell, Eduard Ayguadé, George Teodoro, Alba Melo.
</tr>
<tr>
<td><a href="http://dx.doi.org/10.1109/TPDS.2016.2515597"><font size=1>[TPDS2016]</font></a></td>
<td><sub>Genome Wide Alignment in GPU Cluster with Incremental Speculative Traceback. TPDS 2016. Edans Sandes, Guillermo Miranda, Alba Melo, Xavier Martorell, Eduard Ayguadé.</sub>
</td>
</tr>
<tr>
<td><a href="http://dx.doi.org/10.1109/CCGrid.2014.18"><font size=1>[CCGRID2014]</font></a></td>
<td><sub>CUDAlign 3.0: Parallel Biological Sequence Comparison in Large GPU Clusters. CCGRID 2014:160-169. Edans Sandes, Guillermo Miranda, Alba Melo, Xavier Martorell, Eduard Ayguadé.</sub>
</td>
</tr>
<tr>
<td><a href="http://dx.doi.org/10.1145/2555243.2555280"><font size=1>[PPOPP2014]</font></a></td>
<td><sub>Fine-grain parallel megabase sequence comparison with multiple heterogeneous GPUs. PPOPP 2014:383-384. Edans Sandes, Guillermo Miranda, Alba Melo, Xavier Martorell, Eduard Ayguadé
</sub></td>
</tr>
<tr>
<td><a href="http://dx.doi.org/10.1109/TPDS.2012.194"><font size=1>[TPDS2013]</font></a></td>
<td><sub>Retrieving Smith-Waterman Alignments with Optimizations for Megabase Biological Sequences using GPU. TPDS:24:5:1009-1021. Edans Sandes, Alba Melo</sub></td>
<tr>
<td><a href="http://dx.doi.org/10.1109/IPDPS.2011.114"><font size=1>[IPDPS2011]</font></a></td>
<td><sub>Smith-Waterman Alignment of Huge Sequences with GPU in Linear Space. IPDPS 2011: 1199-1211. Edans Sandes, Alba Melo</sub></td>
</tr>
</tr>
<td><a href="http://dx.doi.org/10.1145/1693453.1693473"><font size=1>[PPOPP2010]</font></a></td>
<td><sub>CUDAlign: using GPU to accelerate the comparison of megabase genomic sequences. PPOPP 2010: 137-146. Edans Sandes, Alba Melo</sub></td>
</tr>

