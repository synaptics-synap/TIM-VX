/****************************************************************************
*
*    Copyright (c) 2022 Synaptics Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#include "graph_synap.h"
#include "synap/ebg_utils.h"

#include "vsi_nn_pub.h"

#include <fstream>
#include <iostream>

using namespace std;

namespace tim {
namespace vx {

GraphSynap::~GraphSynap()
{
    if (ebg_buffer_) free(ebg_buffer_);
}

static string to_json(const vector<size_t>& sizes)
{
    string sep, s = "{";
    int i = 0;
    for (const auto& size: sizes) {
        s += sep + '"' + to_string(i++) + R"(":{"dtype":"byte","shape":[)" + to_string(size) + "]}";
        sep = ",";
    }
    return s + '}';
}


static string to_json(const vector<size_t>& inputs, const vector<size_t>& outputs)
{
    return "{\"Inputs\": " + to_json(inputs) + ",\"Outputs\": " + to_json(outputs) + "}";
}


static vector<size_t> tensor_sizes(const vector<shared_ptr<Tensor>>& tensors)
{
    vector<size_t> sizes;
    for (auto tensor: tensors) {
        sizes.push_back(tensor->GetSpec().GetByteSize());
    }
    return sizes;
}

bool GraphSynap::SetCachePath(const std::string& cachepath)
{
    if (!cachepath.empty()) {
      cache_path_ = cachepath + ".ebg";
      VSILOGD("Set ebg name %s", cache_path_.c_str());
      return true;
    }
    return false;
}

bool GraphSynap::CompileToBinary(void* buf, size_t* size)
{
    if (!buf && !size) {
        if (!cache_path_.empty()) {
          ifstream f(cache_path_.c_str(), ios::binary);
          if (!f.good()) {
            VSILOGE("cannot read cached ebg file %s", cache_path_.c_str());
            return false;
          }
          vector<uint8_t> file_content(istreambuf_iterator<char>{f}, {});

          ebg_size_ = file_content.size();
          if (ebg_size_ <= 0) {
            VSILOGE("cached ebg file size %d not valid", ebg_size_);
            return false;
          }

          ebg_buffer_ = (uint8_t *)malloc(ebg_size_);
          if (!ebg_buffer_) {
            VSILOGE("allocat ebg buffer failed");
            return false;
          }

          memcpy(ebg_buffer_, file_content.data(), ebg_size_);
          return true;
        } else {
          VSILOGE("Setup graph failed");
          return false;
        }
    }

    if (!Setup()) {
        VSILOGE("Setup graph failed");
        return false;
    }

    if (size) {
        if (VSI_SUCCESS != vsi_nn_GenerateNBG(graph_, nullptr, size)) {
            VSILOGE("Error getting NBG size");
            return false;
        }
        if (*size == 0) {
            VSILOGE("Error NBG has size 0");
            return false;
        }
    }
    VSILOGD("compile nbg_size: %zu", *size);

    if (buf == nullptr) {
        // we support the case of only return model size
        return true;
    }

    if (VSI_SUCCESS != vsi_nn_GenerateNBG(graph_, buf, size)) {
        VSILOGE("Error compiling graph to NBG");
        return false;
    }
    VSILOGD("CompileToBinary done");

    ebg_size_ = nbg_to_ebg((uint8_t *)buf, *size, &ebg_buffer_, false);
    if (ebg_size_ == 0 || ebg_buffer_ == nullptr) {
        VSILOGE("NBG to EBG conversion failed");
        return false;
    }
    VSILOGD("NBG to EBG conversion done, ebg_size=%d", ebg_size_);

    if (!cache_path_.empty()) {
        ofstream fs;
        fs.open(cache_path_.c_str(), ios::out | ios::binary);
        fs.write(reinterpret_cast<const char*>(ebg_buffer_), ebg_size_);
        fs.close();
    }

    return true;
}

bool GraphSynap::Compile()
{
    if (!ebg_buffer_) {
        size_t nbg_size = 0;
        if (!this->CompileToBinary(nullptr, &nbg_size)) {
            VSILOGE("compile to binary failed");
            return false;
        }
        vector<uint8_t> nbg_buf(nbg_size);
        if (!this->CompileToBinary(nbg_buf.data(), &nbg_size)) {
            VSILOGE("compile to binary failed");
            return false;
        }
    }

    vector<size_t> input_sizes = tensor_sizes(inputs_tensor_);
    vector<size_t> output_sizes = tensor_sizes(outputs_tensor_);
    if (!network_.load_model(ebg_buffer_, ebg_size_, to_json(input_sizes, output_sizes).c_str())) {
        VSILOGE("Error loading EBG model");
        return false;
    }

    return true;
}


bool GraphSynap::Run() {
    VSILOGD("GraphSynap::Run");
    // Copy data to network input tensors
    size_t ix = 0;
    for (auto& in: network_.inputs) {
        Tensor* t = inputs_tensor_[ix++].get();
        t->CopyDataFromTensor(in.data());
    }

    if (!network_.predict()) {
        VSILOGE("Error executing EBG model");
        return false;
    }

    // Copy data from network output tensors
    ix = 0;
    for (auto& out: network_.outputs) {
        Tensor* t = outputs_tensor_[ix++].get();
        t->CopyDataToTensor(out.data(), out.size());
    }

    return true;
}


}  // namespace vx
}  // namespace tim
