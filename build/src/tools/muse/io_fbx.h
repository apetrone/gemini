// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include "common/extension.h"
#include "common.h"
#include "datamodel/model.h"
#include <fbxsdk.h>

namespace gemini
{

	struct WeightReference
	{
		// datamodel bone index
		uint32_t datamodel_bone_index;
		
		// weight
		float value;
	};
	
	typedef std::vector<WeightReference> WeightReferenceVector;
	struct WeightSlot
	{
		WeightReferenceVector weights;
	};
	
	typedef std::vector<WeightSlot> WeightSlotVector;


	struct NodeData
	{
		FbxAMatrix global_transform;
		datamodel::Node* node;
	};
	typedef std::vector<NodeData> NodeDataVector;


	struct AutodeskFbxExtensionState
	{
		datamodel::Model* model;
		
		// used by the reader to convert to correct units
		// since the SDK will only convert root nodes
		float conversion_factor;
		
		tools::IndentState indent;
		
		
		std::vector<FbxNode*> skeletal_nodes;
		
		WeightSlotVector slots;
		NodeDataVector nodedata;
		float frames_per_second;
	};
	
	class AutodeskFbxReader : public tools::Reader<datamodel::Model>
	{
		DECLARE_PLUGIN_CLASS(AutodeskFbxReader);
		
		AutodeskFbxExtensionState extension_state;
		
	public:
		AutodeskFbxReader();
		virtual ~AutodeskFbxReader();

		virtual void read(datamodel::Model* root, core::util::DataStream& data_source);
	};
} // namespace gemini