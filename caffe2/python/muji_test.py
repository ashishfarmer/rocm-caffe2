# Copyright (c) 2016-present, Facebook, Inc.
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
##############################################################################

import numpy as np
import unittest

from caffe2.python import core, workspace, muji, test_util


@unittest.skipIf(not workspace.has_gpu_support, "no gpu")
class TestMuji(test_util.TestCase):
    def RunningAllreduceWithGPUs(self, gpu_ids, allreduce_function):
        """A base function to test different scenarios."""
        net = core.Net("mujitest")
        for id in gpu_ids:
            net.ConstantFill(
                [],
                "testblob_gpu_" + str(id),
                shape=[1, 2, 3, 4],
                value=float(id + 1),
                device_option=muji.OnGPU(id)
            )
        allreduce_function(
            net, ["testblob_gpu_" + str(i)
                  for i in gpu_ids], "_reduced", gpu_ids
        )
        workspace.RunNetOnce(net)
        target_value = sum(gpu_ids) + len(gpu_ids)
        all_blobs = workspace.Blobs()
        all_blobs.sort()
        for blob in all_blobs:
            print('{} {}'.format(blob, workspace.FetchBlob(blob)))

        for idx in gpu_ids:
            blob = workspace.FetchBlob("testblob_gpu_" + str(idx) + "_reduced")
            np.testing.assert_array_equal(
                blob,
                target_value,
                err_msg="gpu id %d of %s" % (idx, str(gpu_ids))
            )

    def testAllreduceFallback(self):
        self.RunningAllreduceWithGPUs(
            list(range(workspace.NumGpuDevices())), muji.AllreduceFallback
        )

    def testAllreduceSingleGPU(self):
        for i in range(workspace.NumGpuDevices()):
            self.RunningAllreduceWithGPUs([i], muji.Allreduce)

    def testAllreduceWithTwoGPUs(self):
        if workspace.has_hip:
            pattern = workspace.GetHipPeerAccessPattern()
        else:
            pattern = workspace.GetCudaPeerAccessPattern()
        if pattern.shape[0] >= 2 and np.all(pattern[:2, :2]):
            self.RunningAllreduceWithGPUs([0, 1], muji.Allreduce2)
        else:
            print('Skipping allreduce with 2 gpus. Not peer access ready.')

    def testAllreduceWithFourGPUs(self):
        if workspace.has_hip:
            pattern = workspace.GetHipPeerAccessPattern()
        else:
            pattern = workspace.GetCudaPeerAccessPattern()
        if pattern.shape[0] >= 4 and np.all(pattern[:4, :4]):
            self.RunningAllreduceWithGPUs([0, 1, 2, 3], muji.Allreduce4)
        else:
            print('Skipping allreduce with 4 gpus. Not peer access ready.')

    def testAllreduceWithEightGPUs(self):
        if workspace.has_hip:
            pattern = workspace.GetHipPeerAccessPattern()
        else:
            pattern = workspace.GetCudaPeerAccessPattern()
        if (
            pattern.shape[0] >= 8 and np.all(pattern[:4, :4]) and
            np.all(pattern[4:, 4:])
        ):
            self.RunningAllreduceWithGPUs(
                list(range(8)), muji.Allreduce8)
        else:
            print('Skipping allreduce with 8 gpus. Not peer access ready.')
