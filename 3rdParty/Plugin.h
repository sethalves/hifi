#ifndef PLUGIN_H
#define	PLUGIN_H

#include <string>
#include <vector>

#ifdef LEOPLUGIN_DLL_EXPORT
#  define LEOPLUGIN_DLL_API __declspec(dllexport)
#else
#  define LEOPLUGIN_DLL_API __declspec(dllimport)
#endif
LEOPLUGIN_DLL_API
class LeoPlugin
{

public:
    LEOPLUGIN_DLL_API struct EntityIDStruct
    {
        unsigned int data1;
        unsigned short data2, data3;
        unsigned char* data4;
        LEOPLUGIN_DLL_API EntityIDStruct(unsigned int d1, unsigned short d2, unsigned short d3, unsigned char d4[8])
        {
            data1 = d1;
            data2 = d2;
            data3 = d3;
            data4 = new unsigned char[8];
            for (int i = 0; i < 8; i++)
            {
                data4[i] = d4[i];
            }
        }
        LEOPLUGIN_DLL_API EntityIDStruct(){};
        LEOPLUGIN_DLL_API ~EntityIDStruct()
        {
            delete[] data4;
        };
    };
    LEOPLUGIN_DLL_API enum SculptApp_AppState
    {
        APPSTATE_WAIT,
        APPSTATE_GUI_TOOL,
        APPSTATE_ACTIVE_TOOL
    };

    LEOPLUGIN_DLL_API struct IncomingMaterial
    {
        float diffuseColor[4];
        float specularColor[4];
        float emissiveColor[4];
        std::string materialID;
        std::string name;
        std::string diffuseTextureUrl;
    };

    LEOPLUGIN_DLL_API   LeoPlugin();
    LEOPLUGIN_DLL_API   ~LeoPlugin();
    LEOPLUGIN_DLL_API SculptApp_AppState getAppState();
    LEOPLUGIN_DLL_API void SculptApp_exportFile(const char* fileFormat);
    LEOPLUGIN_DLL_API void SculptApp_importFromData(const char* cmdStr);
    LEOPLUGIN_DLL_API void SculptApp_importPattern(
        const char* slotStr,
        const char* data,
        int size);
    LEOPLUGIN_DLL_API void SculptApp_issueAppCmd(const char* cmd, const char* value);
    LEOPLUGIN_DLL_API void SculptApp_addCmdToQueue(const char* cmd, const char* value);
    LEOPLUGIN_DLL_API char* SculptApp_getMaterialInfo();
    LEOPLUGIN_DLL_API bool SculptApp_init();
    LEOPLUGIN_DLL_API int SculptMesh_isModified();
    LEOPLUGIN_DLL_API void SculptApp_ShutoDown();
    LEOPLUGIN_DLL_API void SculptApp_Frame();
    LEOPLUGIN_DLL_API void SculptApp_getMeshInfo(float* data);
    LEOPLUGIN_DLL_API void setControllerStatesInput(short index, unsigned int deviceIndex, double buttons[4], double dPadX, double dPadY, double poseMat[16]);
    LEOPLUGIN_DLL_API void setCameraStateInput(double headPoseMat[16], double eyePoseA[16], double eyePoseB[16], double eyeProjA[16], double eyeProjB[16], double nearDist, double farDist);
    LEOPLUGIN_DLL_API void importFromRawData(float* vertices, unsigned int numVertices, int* indices, unsigned int numIndices, float* normals, unsigned int numNormals, float* texCoords, unsigned int numTexCoords, float worldMat[16], std::vector<IncomingMaterial> metrials,std::vector<unsigned short> triangleMatInds);
    LEOPLUGIN_DLL_API void getSculptMeshNUmberDatas(unsigned int& numVertices, unsigned int& numIndices, unsigned int& numNormals);
    LEOPLUGIN_DLL_API void getRawSculptMeshData(float* vertices, int* indices, float* normals);
    EntityIDStruct CurrentlyUnderEdit;
};
class LeoPolyPlugin
{
public:
    static LeoPlugin& Instance()
    {
        static LeoPlugin inst = LeoPlugin();
        return inst;
    }
};

#endif	/* PLUGIN_H */