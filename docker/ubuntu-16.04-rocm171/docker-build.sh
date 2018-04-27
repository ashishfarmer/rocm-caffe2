#!/bin/bash

# Build the base image
rocm_base_image=rocm-base-"$1"
docker build -f ./docker/ubuntu-16.04-rocm171/rocm-base/Dockerfile --no-cache -t ${rocm_base_image} .
if [ $? -ne 0 ]; then { echo "ERROR: failed base image build!" ; exit 1; } fi

# Build the caffe2 image
caffe2_image=caffe2-"$1"
# docker build -f ./docker/ubuntu-16.04-rocm171/caffe2/Dockerfile --build-arg base_image=${rocm_base_image} --no-cache -t ${caffe2_image} .
docker build -f ./docker/ubuntu-16.04-rocm171/caffe2/Dockerfile --no-cache -t ${caffe2_image} .