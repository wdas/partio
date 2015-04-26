#include <ai.h>
#include <stdio.h>

extern AtNodeMethods* partioPointMtd;

enum SHADERS
{
    SHADER_PARTIOPOINT
};

node_loader
{
	switch(i)
	{
		case SHADER_PARTIOPOINT:
			node->methods     = (AtNodeMethods*) partioPointMtd;
			node->output_type = AI_TYPE_RGBA;
			node->name        = "partioPoint";
			node->node_type   = AI_NODE_SHADER;
			break;
		default:
			return false;
	}

	sprintf(node->version, AI_VERSION);

	return true;
}
