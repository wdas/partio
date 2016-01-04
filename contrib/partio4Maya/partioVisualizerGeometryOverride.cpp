#include "partioVisualizerGeometryOverride.h"

#include <maya/MFnDependencyNode.h>

#include <iostream>
namespace MHWRender{

    MString partioVisualizerGeometryOverride::registrantId("partioVisualizerOverride");

    MHWRender::MPxGeometryOverride* partioVisualizerGeometryOverride::creator(const MObject& object)
    {
        return new partioVisualizerGeometryOverride(object);
    }

    partioVisualizerGeometryOverride::partioVisualizerGeometryOverride(const MObject& object)
            : MHWRender::MPxGeometryOverride(object), m_object(object), p_viz_reader_cache(0)
    {
        p_renderer = MRenderer::theRenderer();
        p_shader_manager = p_renderer->getShaderManager();
    }

    void partioVisualizerGeometryOverride::updateDG()
    {
        MStatus status;
        MFnDependencyNode dnode(m_object, &status);
        p_viz_reader_cache = 0;
        if (status)
        {
            partioVisualizer* visualizer = dynamic_cast<partioVisualizer*>(dnode.userNode());
            if (visualizer != 0)
                p_viz_reader_cache = visualizer->updateParticleCache();
        }
    }

    void partioVisualizerGeometryOverride::updateRenderItems(const MDagPath& path,
                                                             MHWRender::MRenderItemList& list)
    {
        int index = list.indexOf("partioVisualizerPoints");

        if (index < 0)
        {
            MRenderItem* render_item = MRenderItem::Create(
                    "partioVisualizerPoints", MRenderItem::MaterialSceneItem,
                    MGeometry::kPoints);

            render_item->setDrawMode((MGeometry::DrawMode)(MGeometry::kWireframe | MGeometry::kShaded | MGeometry::kTextured));
            render_item->depthPriority(MRenderItem::sDormantPointDepthPriority);

            MShaderInstance* shader = p_shader_manager->getStockShader(MShaderManager::k3dCPVFatPointShader);

            if (shader)
                render_item->setShader(shader);

            list.append(render_item);
        }
    }

    void partioVisualizerGeometryOverride::populateGeometry(const MHWRender::MGeometryRequirements& requirements,
                                                            const MHWRender::MRenderItemList& renderItems,
                                                            MHWRender::MGeometry& data)
    {
        if (p_viz_reader_cache == 0 || p_viz_reader_cache->particles == 0)
            return;

        int index = renderItems.indexOf("partioVisualizerPoints");

        if (index < 0)
            return;

        const MRenderItem* render_item = renderItems.itemAt(index);

        const int num_particles = p_viz_reader_cache->particles->numParticles();

        MHWRender::MVertexBuffer* position_buffer = 0;
        MHWRender::MVertexBuffer* color_buffer = 0;

        float* positions = 0;
        float* colors = 0;

        const MVertexBufferDescriptorList& vertex_reqs = requirements.vertexRequirements();
        const int num_vertex_reqs = vertex_reqs.length();
        for (int req = 0; req < num_vertex_reqs; ++req)
        {
            MVertexBufferDescriptor desc;
            if (!vertex_reqs.getDescriptor(req, desc))
                continue;

            switch (desc.semantic())
            {
                case MGeometry::kPosition:
                    if (position_buffer == 0) // this check is coming from the maya doc example
                    {
                        position_buffer = data.createVertexBuffer(desc);
                        if (position_buffer)
                            positions = reinterpret_cast<float*>(position_buffer->acquire(num_particles, true));
                    }
                    break;
                case MGeometry::kColor: // this is 4 components
                    if (color_buffer == 0) // this check is coming from the maya doc example
                    {
                        color_buffer = data.createVertexBuffer(desc);
                        if (color_buffer)
                            colors = reinterpret_cast<float*>(color_buffer->acquire(num_particles, true));
                    }
                    break;
                default:
                    std::cout << "[partio_visualizer] Requesting unsupported channel!" << std::endl;
                    break;
            }
        }

        if (position_buffer != 0)
        {
            int id = 0;
            for (int i = 0; i < num_particles; ++i)
            {
                const float* pos = p_viz_reader_cache->particles->data<float>(p_viz_reader_cache->positionAttr, i);
                positions[id++] = pos[0];
                positions[id++] = pos[1];
                positions[id++] = pos[2];
            }
            position_buffer->commit(positions);
        }

        if (color_buffer != 0)
        {
            int id = 0;
            for (int i = 0; i < num_particles; ++i)
            {
                colors[id++] = p_viz_reader_cache->rgba[i * 4];
                colors[id++] = p_viz_reader_cache->rgba[i * 4 + 1];
                colors[id++] = p_viz_reader_cache->rgba[i * 4 + 2];
                colors[id++] = p_viz_reader_cache->rgba[i * 4 + 3];
            }
            color_buffer->commit(colors);
        }

        MHWRender::MIndexBuffer* index_buffer = data.createIndexBuffer(MGeometry::kUnsignedInt32);

        if (index_buffer)
        {
            unsigned int* indices = reinterpret_cast<unsigned int*>(index_buffer->acquire(num_particles));
            if (indices != 0)
            {
                for (int i = 0; i < num_particles; ++i)
                    indices[i] = static_cast<unsigned int>(i);

                index_buffer->commit(indices);

                render_item->associateWithIndexBuffer(index_buffer);
            }
        }
    }

    void partioVisualizerGeometryOverride::cleanUp()
    {

    }


}
