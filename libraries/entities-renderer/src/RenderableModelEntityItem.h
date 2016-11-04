//
//  RenderableModelEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableModelEntityItem_h
#define hifi_RenderableModelEntityItem_h

#include <QString>
#include <QStringList>

#include <ModelEntityItem.h>
#include <Plugin.h>

class Model;
class EntityTreeRenderer;

class RenderableModelEntityItem : public ModelEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableModelEntityItem(const EntityItemID& entityItemID, bool dimensionsInitialized);

    virtual ~RenderableModelEntityItem();

    virtual void setDimensions(const glm::vec3& value) override;
    virtual void setModelURL(const QString& url) override;

    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;
    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    void doInitialModelSimulation();

    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override;
    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override;


    void updateModelBounds();
    virtual void render(RenderArgs* args) override;
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        bool& keepSearching, OctreeElementPointer& element, float& distance,
                        BoxFace& face, glm::vec3& surfaceNormal,
                        void** intersectedObject, bool precisionPicking) const override;
    ModelPointer getModel(QSharedPointer<EntityTreeRenderer> renderer);

    virtual bool needsToCallUpdate() const override;
    virtual void update(const quint64& now) override;

    virtual void setShapeType(ShapeType type) override;
    virtual void setCompoundShapeURL(const QString& url) override;

    virtual bool isReadyToComputeShape() override;
    virtual void computeShapeInfo(ShapeInfo& shapeInfo) override;

    void setCollisionShape(const btCollisionShape* shape) override;

    virtual bool contains(const glm::vec3& point) const override;

    virtual bool shouldBePhysical() const override;

    // these are in the frame of this object (model space)
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override;
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override;

    virtual void setJointRotations(const QVector<glm::quat>& rotations) override;
    virtual void setJointRotationsSet(const QVector<bool>& rotationsSet) override;
    virtual void setJointTranslations(const QVector<glm::vec3>& translations) override;
    virtual void setJointTranslationsSet(const QVector<bool>& translationsSet) override;

    virtual void loader() override;
    virtual void locationChanged(bool tellPhysics = true) override;

    virtual void resizeJointArrays(int newSize = -1) override;

    virtual int getJointIndex(const QString& name) const override;
    virtual QStringList getJointNames() const override;

    // These operate on a copy of the animationProperties, so they can be accessed
    // without having the entityTree lock.
    bool hasRenderAnimation() const { return !_renderAnimationProperties.getURL().isEmpty(); }
    const QString& getRenderAnimationURL() const { return _renderAnimationProperties.getURL(); }

    render::ItemID getMetaRenderItem() { return _myMetaItem; }

    // Transparency is handled in ModelMeshPartPayload
    bool isTransparent() override { return false; }

    virtual void setUnderSculpting(bool value)
    {
        if (value)
        {
            QVector<glm::vec3> vertices;
            QVector<glm::vec3> normals;
            QVector<glm::vec2> texCoords;
            QVector<int> indices;
            QVector<std::string> matStringsPerTriangles;
            QVector<unsigned short> matIndexesPerTriangles;
            ModelPointer act = getModel(_myRenderer);
            auto geometry = act->getFBXGeometry();

            std::string baseUrl = act->getURL().toString().toStdString().substr(0, act->getURL().toString().toStdString().find_last_of("\\/"));

            std::vector<LeoPlugin::IncomingMaterial> materialsToSend;
            foreach(const FBXMaterial mat, geometry.materials)
            {
                LeoPlugin::IncomingMaterial actMat;
                if (!mat.albedoTexture.filename.isEmpty() && mat.albedoTexture.content.isEmpty() &&
                    !_textures.contains(mat.albedoTexture.filename))
                {
                    actMat.diffuseTextureUrl = baseUrl + "//" + mat.albedoTexture.filename.toStdString();
                }
                /*if (!mat.normalTexture.filename.isEmpty() && mat.normalTexture.content.isEmpty() &&
                !_textures.contains(mat.normalTexture.filename))
                {

                _texturesURLs.push_back(baseUrl + "\\" + mat.normalTexture.filename.toStdString());
                }
                if (!mat.specularTexture.filename.isEmpty() && mat.specularTexture.content.isEmpty() &&
                !_textures.contains(mat.specularTexture.filename))
                {
                _texturesURLs.push_back(baseUrl + "\\" + mat.specularTexture.filename.toStdString());
                }
                if (!mat.emissiveTexture.filename.isEmpty() && mat.emissiveTexture.content.isEmpty() &&
                !_textures.contains(mat.emissiveTexture.filename))
                {
                _texturesURLs.push_back(baseUrl + "\\" + mat.emissiveTexture.filename.toStdString());
                }*/
                for (int i = 0; i < 3; i++)
                {
                    actMat.diffuseColor[i] = mat.diffuseColor[i];
                    actMat.emissiveColor[i] = mat.emissiveColor[i];
                    actMat.specularColor[i] = mat.specularColor[i];
                }
                actMat.diffuseColor[3] = 1;
                actMat.emissiveColor[3] = 0;
                actMat.specularColor[3] = 0;
                actMat.materialID = mat.materialID.toStdString();
                actMat.name = mat.name.toStdString();
                materialsToSend.push_back(actMat);
            }
            for (auto actmesh : act->getGeometry()->getFBXGeometry().meshes)
            {
                vertices.append(actmesh.vertices);
                normals.append(actmesh.normals);
                texCoords.append(actmesh.texCoords);
                for (auto subMesh : actmesh.parts)
                {
                    int startIndex = indices.size();
                    if (subMesh.triangleIndices.size() > 0)
                    {
                        indices.append(subMesh.triangleIndices);
                    }
                    if (subMesh.quadTrianglesIndices.size() > 0)
                    {
                        indices.append(subMesh.quadTrianglesIndices);
                    }
                    else
                        if (subMesh.quadIndices.size() > 0)
                        {
                            assert(subMesh.quadIndices.size() % 4 == 0);
                            for (int i = 0; i < subMesh.quadIndices.size() / 4; i++)
                            {
                                indices.push_back(subMesh.quadIndices[i * 4]);
                                indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                                indices.push_back(subMesh.quadIndices[i * 4 + 2]);

                                indices.push_back(subMesh.quadIndices[i * 4 + 3]);
                                indices.push_back(subMesh.quadIndices[i * 4]);
                                indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                            }
                        }
                    for (int i = 0; i < (indices.size() - startIndex) / 3; i++)
                    {
                        matStringsPerTriangles.push_back(subMesh.materialID.toStdString());
                    }

                }
            }
            float* verticesFlattened = new float[vertices.size() * 3];
            float *normalsFlattened = new float[normals.size() * 3];
            float *texCoordsFlattened = new float[texCoords.size() * 2];
            int *indicesFlattened = new int[indices.size()];
            for (int i = 0; i < vertices.size(); i++)
            {

                verticesFlattened[i * 3] = vertices[i].x;
                verticesFlattened[i * 3 + 1] = vertices[i].y;
                verticesFlattened[i * 3 + 2] = vertices[i].z;

                normalsFlattened[i * 3] = normals[i].x;
                normalsFlattened[i * 3 + 1] = normals[i].y;
                normalsFlattened[i * 3 + 2] = normals[i].z;

                texCoordsFlattened[i * 2] = texCoords[i].x;
                texCoordsFlattened[i * 2 + 1] = texCoords[i].y;
            }
            for (int i = 0; i < indices.size(); i++)
            {
                indicesFlattened[i] = indices[i];
            }
            matIndexesPerTriangles.resize(matStringsPerTriangles.size());
            for (int matInd = 0; matInd < materialsToSend.size(); matInd++)
            {
                for (int i = 0; i < matStringsPerTriangles.size(); i++)
                {
                    if (matStringsPerTriangles[i] == materialsToSend[matInd].materialID)
                    {
                        matIndexesPerTriangles[i] = matInd;
                    }
                }
            }
            LeoPolyPlugin::Instance().importFromRawData(verticesFlattened, vertices.size(), indicesFlattened, indices.size(), normalsFlattened, normals.size(), texCoordsFlattened, texCoords.size(),const_cast<float*>(glm::value_ptr(glm::transpose(getTransform().getMatrix()))), materialsToSend, matIndexesPerTriangles.toStdVector());
            delete[] verticesFlattened;
            delete[] indicesFlattened;
            delete[] normalsFlattened;
            delete[] texCoordsFlattened;
        }

    }

private:
    QVariantMap parseTexturesToMap(QString textures);
    void remapTextures();

    GeometryResource::Pointer _compoundShapeResource;
    ModelPointer _model = nullptr;
    bool _needsInitialSimulation = true;
    bool _needsModelReload = true;
    QSharedPointer<EntityTreeRenderer> _myRenderer;
    QString _lastTextures;
    QVariantMap _currentTextures;
    QVariantMap _originalTextures;
    bool _originalTexturesRead = false;
    bool _dimensionsInitialized = true;

    AnimationPropertyGroup _renderAnimationProperties;

    render::ItemID _myMetaItem{ render::Item::INVALID_ITEM_ID };

    bool getAnimationFrame();

    bool _needsJointSimulation { false };
    bool _showCollisionGeometry { false };
    const void* _collisionMeshKey { nullptr };
};

#endif // hifi_RenderableModelEntityItem_h
