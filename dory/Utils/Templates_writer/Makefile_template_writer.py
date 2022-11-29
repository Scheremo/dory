#
# template.py
# Alessio Burrello <alessio.burrello@unibo.it>
# Thorir Mar Ingolfsson <thoriri@iis.ee.ethz.ch>
#
# Copyright (C) 2019-2020 University of Bologna
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from mako.template import Template
from collections import OrderedDict
import os


def print_template_Makefile(
    graph,
    HW_description,
    save_string,
    app_directory):
    # Generate the Makefile, including all files to upload on the hyperflash
    tk = OrderedDict([])
    file_list_w = []
    for i, node in enumerate(graph):
        if "Conv" in node.name or "FullyConnected" in node.name:
            file_list_w.append(node.name+"_weights.hex")

    tk['n_inputs'] = graph[0].n_test_inputs
    tk['layers_w'] = file_list_w
    tk['sdk'] = HW_description["software development kit"]["name"]
    tk['do_flash'] = HW_description["memory"]["levels"] > 2
    try:
        blocking_dma_transfers = HW_description['blocking_dma_transfers']
    except KeyError:
        print("Makefile template writer: key 'always_blocking_dma_transfers' not found in HW description, using non-blocking transfers!")
        blocking_dma_transfers = False
    tk['blocking_dma'] = blocking_dma_transfers
    root = os.path.realpath(os.path.dirname(__file__))
    tmpl = Template(filename=os.path.join(root, "../../Hardware_targets", HW_description["name"], "Templates/Makefile_template"))
    s = tmpl.render(**tk)
    save_string = os.path.join(app_directory, save_string)
    with open(save_string, "w") as f:
        f.write(s)
