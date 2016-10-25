//
//  RenderableLeoPolyObjectEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableLeoPolyObjectEntityItem_h
#define hifi_RenderableLeoPolyObjectEntityItem_h

#include <QSemaphore>
#include <atomic>

#include <PolyVoxCore/SimpleVolume.h>
#include <PolyVoxCore/Raycast.h>

#include <TextureCache.h>

#include "PolyVoxEntityItem.h"
#include "RenderableEntityItem.h"
#include "gpu/Context.h"
#include <Plugin.h>
#include "RenderablePolyVoxEntityItem.h"
#include "LeoPolyObjectEntityItem.h"

class LeoPolyObjectPayload {
public:
    LeoPolyObjectPayload(EntityItemPointer owner) : _owner(owner), _bounds(AABox()) { }
    typedef render::Payload<LeoPolyObjectPayload> Payload;
    typedef Payload::DataPointer Pointer;

    EntityItemPointer _owner;
    AABox _bounds;
};

namespace render {
    template <> const ItemKey payloadGetKey(const LeoPolyObjectPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const LeoPolyObjectPayload::Pointer& payload);
    template <> void payloadRender(const LeoPolyObjectPayload::Pointer& payload, RenderArgs* args);
}



class RenderableLeoPolyObjectEntityItem : public LeoPolyObjectEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableLeoPolyObjectEntityItem(const EntityItemID& entityItemID);
    void updateGeometryFromLeoPlugin()
    {
       
        //if (_mesh)
        {

            //removeFromScene
            float* vertices, *normals;
            int *indices;
            unsigned int numVertices, numNormals, numIndices;
            LeoPolyPlugin::Instance().getSculptMeshNUmberDatas(numVertices, numIndices, numNormals);
            vertices = new float[numVertices*3];
            normals = new float[numNormals*3];
            indices = new int[numIndices];
            LeoPolyPlugin::Instance().getRawSculptMeshData(vertices, indices, normals);


         
            float scaleGuess = 1.0f;

            bool needsMaterialLibrary = false;

            // call parseOBJGroup as long as it's returning true.  Each successful call will
            // add a new meshPart to the geometry's single mesh.

//            delete _mesh.get();
            FBXMesh* mesh =new FBXMesh();
            mesh->meshIndex = 0;

            FBXCluster cluster;
            cluster.jointIndex = 0;
            cluster.inverseBindMatrix = glm::mat4(1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
            mesh->clusters.append(cluster);
            mesh->parts.push_back(FBXMeshPart());
            for (int i = 0; i < numVertices; i++)
            {
                mesh->vertices << glm::vec3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
            }
            for (int i = 0; i < numIndices; i++)
            {
                mesh->parts[0].triangleIndices << indices[i];
            }
            for (int i = 0; i < numNormals; i++)
            {
                mesh->normals << glm::vec3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
            }
            //                for (int i = 0, meshPartCount = 0; i < mesh.parts.count(); i++, meshPartCount++) 
            //                {
            //                    FBXMeshPart& meshPart = mesh.parts[i];
            //                    FaceGroup faceGroup = faceGroups[meshPartCount];
            //                    bool specifiesUV = false;
            //                    foreach(OBJFace face, faceGroup) {
            //                        glm::vec3 v0 = checked_at(vertices, face.vertexIndices[0]);
            //                        glm::vec3 v1 = checked_at(vertices, face.vertexIndices[1]);
            //                        glm::vec3 v2 = checked_at(vertices, face.vertexIndices[2]);
            //                        meshPart.triangleIndices.append(mesh.vertices.count()); // not face.vertexIndices into vertices
            //                        mesh.vertices << v0;
            //                        meshPart.triangleIndices.append(mesh.vertices.count());
            //                        mesh.vertices << v1;
            //                        meshPart.triangleIndices.append(mesh.vertices.count());
            //                        mesh.vertices << v2;
            //
            //                        glm::vec3 n0, n1, n2;
            //                        if (face.normalIndices.count()) {
            //                            n0 = checked_at(normals, face.normalIndices[0]);
            //                            n1 = checked_at(normals, face.normalIndices[1]);
            //                            n2 = checked_at(normals, face.normalIndices[2]);
            //                        }
            //                        else { // generate normals from triangle plane if not provided
            //                            n0 = n1 = n2 = glm::cross(v1 - v0, v2 - v0);
            //                        }
            //                        mesh.normals << n0 << n1 << n2;
            //                        if (face.textureUVIndices.count()) {
            //                            specifiesUV = true;
            //                            mesh.texCoords <<
            //                                checked_at(textureUVs, face.textureUVIndices[0]) <<
            //                                checked_at(textureUVs, face.textureUVIndices[1]) <<
            //                                checked_at(textureUVs, face.textureUVIndices[2]);
            //                        }
            //                        else {
            //                            glm::vec2 corner(0.0f, 1.0f);
            //                            mesh.texCoords << corner << corner << corner;
            //                        }
            //                    }
            //                    // All the faces in the same group will have the same name and material.
            //                    OBJFace leadFace = faceGroup[0];
            //                    QString groupMaterialName = leadFace.materialName;
            //                    if (groupMaterialName.isEmpty() && specifiesUV) {
            //#ifdef WANT_DEBUG
            //                        qCDebug(modelformat) << "OBJ Reader WARNING: " << url
            //                            << " needs a texture that isn't specified. Using default mechanism.";
            //#endif
            //                        groupMaterialName = SMART_DEFAULT_MATERIAL_NAME;
            //                    }
            //                    if (!groupMaterialName.isEmpty()) {
            //                        OBJMaterial& material = materials[groupMaterialName];
            //                        if (specifiesUV) {
            //                            material.userSpecifiesUV = true; // Note might not be true in a later usage.
            //                        }
            //                        if (specifiesUV || (0 != groupMaterialName.compare("none", Qt::CaseInsensitive))) {
            //                            // Blender has a convention that a material named "None" isn't really used (or defined).
            //                            material.used = true;
            //                            needsMaterialLibrary =false;
            //                        }
            //                        materials[groupMaterialName] = material;
            //                        meshPart.materialID = groupMaterialName;
            //                    }
            //
            //                }
            //
            // if we got a hint about units, scale all the points
            if (scaleGuess != 1.0f) {
                for (int i = 0; i < mesh->vertices.size(); i++) {
                    mesh->vertices[i] *= scaleGuess;
                }
            }

            mesh->meshExtents.reset();
            for(int i = 0; i < mesh->vertices.size();i++) {
                mesh->meshExtents.addPoint(mesh->vertices[i]);
            }

            FBXReader::buildModelMesh(*mesh, "");
           
            setMesh(mesh->_mesh);
           
            //_mesh =model::MeshPointer(mesh->_mesh);
            _meshDirty = true;
            delete[] vertices;
            delete[] normals;
            delete[] indices;
            //rig
            // auto x = FBXGeometry::Pointer(geometryPtr);
            //                _model->getGeometry().reset(x)
        }

    }
    virtual ~RenderableLeoPolyObjectEntityItem();

    void initializePolyVox();

    virtual void somethingChangedNotification() override {
        // This gets called from EnityItem::readEntityDataFromBuffer every time a packet describing
        // this entity comes from the entity-server.  It gets called even if nothing has actually changed
        // (see the comment in EntityItem.cpp).  If that gets fixed, this could be used to know if we
        // need to redo the voxel data.
        // _needsModelReload = true;
    }

    virtual uint8_t getVoxel(int x, int y, int z) override;
    virtual bool setVoxel(int x, int y, int z, uint8_t toValue) override;

    void render(RenderArgs* args) override;
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        bool& keepSearching, OctreeElementPointer& element, float& distance, 
                        BoxFace& face, glm::vec3& surfaceNormal,
                        void** intersectedObject, bool precisionPicking) const override;

    virtual void setLeoPolyURLData(QByteArray LeoPolyURLData) override;
    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize) override;
    virtual void setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) override;

    glm::vec3 getSurfacePositionAdjustment() const;
    glm::mat4 voxelToWorldMatrix() const;
    glm::mat4 worldToVoxelMatrix() const;
    glm::mat4 voxelToLocalMatrix() const;
    glm::mat4 localToVoxelMatrix() const;

    virtual ShapeType getShapeType() const override;
    virtual bool shouldBePhysical() const override { return !isDead(); }
    virtual bool isReadyToComputeShape() override;
    virtual void computeShapeInfo(ShapeInfo& info) override;

    virtual glm::vec3 voxelCoordsToWorldCoords(glm::vec3& voxelCoords) const override;
    virtual glm::vec3 worldCoordsToVoxelCoords(glm::vec3& worldCoords) const override;
    virtual glm::vec3 voxelCoordsToLocalCoords(glm::vec3& voxelCoords) const override;
    virtual glm::vec3 localCoordsToVoxelCoords(glm::vec3& localCoords) const override;

    // coords are in voxel-volume space
    virtual bool setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue) override;
    virtual bool setVoxelInVolume(glm::vec3 position, uint8_t toValue) override;

    // coords are in world-space
    virtual bool setSphere(glm::vec3 center, float radius, uint8_t toValue) override;
    virtual bool setAll(uint8_t toValue) override;
    virtual bool setCuboid(const glm::vec3& lowPosition, const glm::vec3& cuboidSize, int toValue) override;

    virtual void setXTextureURL(QString xTextureURL) override;
    virtual void setYTextureURL(QString yTextureURL) override;
    virtual void setZTextureURL(QString zTextureURL) override;

    virtual bool addToScene(EntityItemPointer self,
                            std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges) override;
    virtual void removeFromScene(EntityItemPointer self,
                                 std::shared_ptr<render::Scene> scene,
                                 render::PendingChanges& pendingChanges) override;

    virtual void setXNNeighborID(const EntityItemID& xNNeighborID) override;
    virtual void setYNNeighborID(const EntityItemID& yNNeighborID) override;
    virtual void setZNNeighborID(const EntityItemID& zNNeighborID) override;

    virtual void setXPNeighborID(const EntityItemID& xPNeighborID) override;
    virtual void setYPNeighborID(const EntityItemID& yPNeighborID) override;
    virtual void setZPNeighborID(const EntityItemID& zPNeighborID) override;

    virtual void updateRegistrationPoint(const glm::vec3& value) override;

    void setVoxelsFromData(QByteArray uncompressedData, quint16 voxelXSize, quint16 voxelYSize, quint16 voxelZSize);
    void forEachVoxelValue(quint16 voxelXSize, quint16 voxelYSize, quint16 voxelZSize,
                           std::function<void(int, int, int, uint8_t)> thunk);

    void setMesh(model::MeshPointer mesh);
    void setCollisionPoints(ShapeInfo::PointCollection points, AABox box);
    PolyVox::SimpleVolume<uint8_t>* getVolData() { return _volData; }

    uint8_t getVoxelInternal(int x, int y, int z);
    bool setVoxelInternal(int x, int y, int z, uint8_t toValue);

    void setVolDataDirty() { withWriteLock([&] { _volDataDirty = true; }); }

    // Transparent polyvox didn't seem to be working so disable for now
    bool isTransparent() override { return false; }
    void compressVolumeDataAndSendEditPacket();
private:
    // The PolyVoxEntityItem class has _voxelData which contains dimensions and compressed voxel data.  The dimensions
    // may not match _voxelVolumeSize.

    model::MeshPointer _mesh;
    bool _meshDirty { true }; // does collision-shape need to be recomputed?
    bool _meshInitialized { false };

    NetworkTexturePointer _xTexture;
    NetworkTexturePointer _yTexture;
    NetworkTexturePointer _zTexture;

    const int MATERIAL_GPU_SLOT = 3;
    render::ItemID _myItem{ render::Item::INVALID_ITEM_ID };
    static gpu::PipelinePointer _pipeline;

    ShapeInfo _shapeInfo;

    PolyVox::SimpleVolume<uint8_t>* _volData = nullptr;
    bool _volDataDirty = false; // does getMesh need to be called?
    int _onCount; // how many non-zero voxels are in _volData

    bool _neighborsNeedUpdate { false };

    bool updateOnCount(int x, int y, int z, uint8_t toValue);
    PolyVox::RaycastResult doRayCast(glm::vec4 originInVoxel, glm::vec4 farInVoxel, glm::vec4& result) const;

    // these are run off the main thread
    void decompressVolumeData();
    
    virtual void getMesh() override; // recompute mesh
    void computeShapeInfoWorker();
    static bool isEdged(PolyVoxEntityItem::PolyVoxSurfaceStyle surfaceStyle);
    // these are cached lookups of _xNNeighborID, _yNNeighborID, _zNNeighborID, _xPNeighborID, _yPNeighborID, _zPNeighborID
    EntityItemWeakPointer _xNNeighbor; // neighbor found by going along negative X axis
    EntityItemWeakPointer _yNNeighbor;
    EntityItemWeakPointer _zNNeighbor;
    EntityItemWeakPointer _xPNeighbor; // neighbor found by going along positive X axis
    EntityItemWeakPointer _yPNeighbor;
    EntityItemWeakPointer _zPNeighbor;
    void cacheNeighbors();
    void copyUpperEdgesFromNeighbors();
    void bonkNeighbors();
    static bool inUserBounds(const PolyVox::SimpleVolume<uint8_t>* vol, PolyVoxEntityItem::PolyVoxSurfaceStyle surfaceStyle,
        int x, int y, int z);
    static EntityItemPointer lookUpNeighbor(EntityTreePointer tree, EntityItemID neighborID, EntityItemWeakPointer& currentWP);
};



#endif // hifi_RenderableLeoPolyObjectEntityItem_h
