#include "Renderer.h"
using namespace wl::component;

void Renderer::recordRenderCmd(wl::backend::VkRenderPresent& vkrenderPrensent)
{
	vkrenderPrensent.BeginRenderPassCmd(*renderPass_sptr, clearValues, *frameBuffers_sptr);
	vkrenderPrensent.BindPipelineCmd(*pipeline_sptr);
	vkrenderPrensent.BindVertexCmd(mesh_sptr->vertexBuffer.vkBuffer);
	vkrenderPrensent.BindIndexCmd(mesh_sptr->indiceBuffer.vkBuffer);
	vkCmdBindDescriptorSets(
		vkrenderPrensent.commandBuffers[vkrenderPrensent.currentFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		*pipelineLayout_sptr,
		0,
		dscrptorSets_sptr->size(),
		dscrptorSets_sptr->data(),
		0,
		nullptr
		);
	vkrenderPrensent.IndexDrawCmd(mesh_sptr->indices.size());
	vkrenderPrensent.EndRenderPassCmd();
}

//Renderer& Renderer::operator=(const Renderer& other)
//{
//	// TODO: 在此处插入 return 语句
//	this->mesh_sptr = other.mesh_sptr;
//	return *this;
//}
