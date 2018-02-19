struct Bound {
	float3 min;
	float padding0;
	float3 max;
	float padding1;
};
 
struct TreeNode {
	int left;   // primitive index if is leaf
	int right;  // == 0 if is leaf
	int parent;
	int padding;
	Bound bound;
};

// 对应于主机端的constant buffer  
cbuffer RadixCB : register(b0)
{  
	int node_count;
};  
  
// 对应于主机端的Shader Resource View  
//StructuredBuffer<BufType> buffer0 : register(t0);  
//StructuredBuffer<BufType> buffer1 : register(t1);  

// 对应于主机端的Unordered Access View  
RWStructuredBuffer<TreeNode> bufferOut : register(u0);



int longestCommonPrefix(int i, int j, int len) {
	if (0 <= j && j < len) {
		return 31 - firstbithigh(i ^ j);
	}
	else {
		return -1;
	}
}




[numthreads(64, 1, 1)]
void CSMain(uint3 groupID : SV_GroupID, uint3 tid : SV_DispatchThreadID,   
    uint3 localTID : SV_GroupThreadID, uint gIdx : SV_GroupIndex)  
{  
    const int i = tid.x;

	if (i >= node_count) {
		return;
	}
	int d = longestCommonPrefix(i, i + 1, node_count + 1) -
		longestCommonPrefix(i, i - 1, node_count + 1) > 0 ? 1 : -1;

	// Compute upper bound for the length of the range
	int sigMin = longestCommonPrefix(i, i - d, node_count + 1);
	int lmax = 2;

	while (longestCommonPrefix(i, i + lmax * d, node_count + 1) > sigMin) {
		lmax *= 2;
	}

	// Find the other end using binary search
	int l = 0;
	int divider = 2;
	for (int t = lmax / divider; t >= 1; divider *= 2) {
		if (longestCommonPrefix(i, i + (l + t) * d, node_count + 1) > sigMin) {
			l += t;
		}
		t = lmax / divider;
	}

	int j = i + l * d;


	// Find the split position using binary search
	int sigNode = longestCommonPrefix(i, j, node_count + 1);
	int s = 0;
	divider = 2;
	for (int t = (l + (divider - 1)) / divider; t >= 1; divider *= 2) {
		if (longestCommonPrefix(i, i + (s + t) * d, node_count + 1) > sigNode) {
			s = s + t;
		}
		t = (l + (divider - 1)) / divider;
	}

	int gamma = i + s * d + min(d, 0);

	if (min(i, j) == gamma) {
		bufferOut[i].left = gamma + node_count;
		bufferOut[node_count + gamma].parent = i;
	}
	else {
		bufferOut[i].left = gamma;
		bufferOut[gamma].parent = i;
	}

	if (max(i, j) == gamma + 1) {
		bufferOut[i].right = node_count + gamma + 1;
		bufferOut[node_count + gamma + 1].parent = i;
	}
	else {
		bufferOut[i].right = gamma + 1;
		bufferOut[gamma + 1].parent = i;
	}

	//current->min = intMin(i, j);
	//current->max = intMax(i, j);
}  
