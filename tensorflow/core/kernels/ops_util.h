/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_KERNELS_OPS_UTIL_H_
#define TENSORFLOW_KERNELS_OPS_UTIL_H_

// This file contains utilities for various operations.

#include <array>

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/util/padding.h"

namespace tensorflow {

// Get2dOutputSize(): Given an input tensor, kernel, stride and padding
// type, the function computes the output and padding dimensions.
//
// Convolution layers take in an input tensor of shape (D, C, R, B), and
// convolve it with a set of filters, which can also be presented as a
// tensor (D, K, K, M), where M is the number of filters, K is the filter size,
// and each 3-dimensional tensor of size (D, K, K) is a filter. For
// simplicity we assume that we always use square filters (which is usually the
// case in images). It also takes in a few additional parameters:
//
// Stride (S): the stride with which we apply the filters. This is the offset
// between locations where we apply the filters. A larger stride
// means that the output will be spatially smaller.
//
// Padding (P): the padding we apply to the input tensor along the R and C
// dimensions. This is usually used to make sure that the spatial dimension
// do not shrink when we progress with convolutions. Two types of padding are
// often used:
//   SAME: the pad value is computed so that the output will have size R/S
//         and C/S.
//   VALID: no padding is carried out.
// The padded area is zero-filled.
//
// The output dimensions for convolution and many other operations, when given
// all the parameters above, are as follows:
// - When Padding = SAME: the output size is (B, R', C', M), where
//     R' = ceil(float(R) / float(S))
//     C' = ceil(float(C) / float(S))
//   where ceil is the ceiling function. The number of padded rows and columns
//   are computed as:
//     Pr = ((R' - 1) * S + K - R) / 2
//     Pc = ((C' - 1) * S + K - C) / 2
//   When the stride is 1, we have the simplified case
//     R'=R, C'=C, Pr=Pc=(K-1)/2.
//   This is where SAME comes from - the output has the same size as the input
//   has.
//
// - When Padding = VALID: the output size is computed as
//     R' = ceil(float(R - K + 1) / float(S))
//     C' = ceil(float(C - K + 1) / float(S))
//   and the number of padded rows and columns are computed in the same way.
//   When the stride is 1, we have the simplified case
//     R'=R-K+1, C'=C-K+1, Pr=0, Pc=0.
//
// For convolution, mathematically, the output value at location (b, r', c', m)
// is the inner product of two vectors: the chunk of input at
//    (b, (r'*S-Pr) : (r'*S-Pr+K), (c'*S-Pc) : (c'*S-Pc+K), :),
// and the filter at (m, :, :, :).
//
Status Get2dOutputSize(const int in_height, const int in_width,
                       int filter_height, int filter_width, int row_stride,
                       int col_stride, Padding padding, int* new_height,
                       int* new_width, int* pad_rows, int* pad_cols);

// Returns the same output dimensions as in Get2dOutputSize, but returns verbose
// padding dimensions (top/bottom/left/right). Any excess padding (caused by
// an odd padding size value) is added to the 'pad_bottom' and 'pad_right'
// dimensions.
Status Get2dOutputSizeVerbose(const int in_height, const int in_width,
                              int filter_height, int filter_width,
                              int row_stride, int col_stride, Padding padding,
                              int* new_height, int* new_width, int* pad_top,
                              int* pad_bottom, int* pad_left, int* pad_right);

// Given an input tensor, kernel, stride and padding type, populates the 3D size
// of the output tensor and padding to be applied to the input tensor at the
// lower end of every dimension. Use for 3D convolutions, where the input data
// is padded with zeros, as well as for 3D avg/max pooling, where the input data
// is padded with invalid values that are not considered for pooling.
//
// TODO(mjanusz): Unify this with Get2dOutputSize by using a common template.
Status Get3dOutputSize(const std::array<int64, 3>& input,
                       const std::array<int64, 3>& window,
                       const std::array<int64, 3>& strides,
                       Padding padding_type, std::array<int64, 3>* output,
                       std::array<int64, 3>* padding);

// Calculates broadcast starting index and size.  For SAME padding, addition
// padding could be applied to right, left, top and bottom.  Depending on the
// current index, input size, kernel size, stride, padding size, the starting
// index and size for broadcast for that dimension are different from the
// current index and kernel size.
// This is mainly used by gradient algorithms for pooling operations.
Status GetBroadcastSize(const int index, const int in_size, const int ksize,
                        const int stride, const int pad_size, int* bindex,
                        int* bsize);

// Converts Brain's Padding to Eigen's PaddingType.
Eigen::PaddingType BrainPadding2EigenPadding(Padding padding);

// Given a shape 's' of a tensor of type T. Returns true iff the
// number of bytes occupied by each dim 0 (i.e., &tensor(i + 1, ...) -
// &tensor(i, ...)) is multiple of EIGEN_MAX_ALIGN_BYTES.
template <typename T>
bool IsInnerDimsSizeAligned(const TensorShape& s) {
  if (s.dims() == 0) return false;
  const int64 dim0_size = s.dim_size(0);
  if (dim0_size == 0) return false;
  const int64 bytes_per_dim0 = (s.num_elements() / dim0_size) * sizeof(T);
  return bytes_per_dim0 % EIGEN_MAX_ALIGN_BYTES == 0;
}

// Returns <suffix> sanitized to have only [a-zA-Z0-9-_].
string SanitizeThreadSuffix(string suffix);

}  // namespace tensorflow

#endif  // TENSORFLOW_KERNELS_OPS_UTIL_H_
