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
    //Struct for storing entity datas of the sculpted object
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
    //LeoEngine states
    //Wait for example when importing/exporting... etc
    //GUI on gui event
    //Active tool by default 
    LEOPLUGIN_DLL_API enum SculptApp_AppState
    {
        APPSTATE_WAIT,
        APPSTATE_GUI_TOOL,
        APPSTATE_ACTIVE_TOOL
    };

    //Struct for syncing the materials
    LEOPLUGIN_DLL_API struct IncomingMaterial
    {
        float diffuseColor[4];
        float specularColor[4];
        float emissiveColor[4];
        std::string materialID;
        std::string name;
        std::string diffuseTextureUrl;
    };

    //simple constructor
    LEOPLUGIN_DLL_API   LeoPlugin();

    //simple destructor
    LEOPLUGIN_DLL_API   ~LeoPlugin();

    //Returns the current appstate enum from the LeoEngine
    LEOPLUGIN_DLL_API SculptApp_AppState getAppState();

    //Starte export process with the current mesh with the given fileName
    LEOPLUGIN_DLL_API void SculptApp_exportFile(const char* fileName);

    //Import from data
    LEOPLUGIN_DLL_API void SculptApp_importFromData(const char* cmdStr);
    
    //Pattern import via texture data with the given slot with given size
    //Can be used for texture stamping or painting
    LEOPLUGIN_DLL_API void SculptApp_importPattern(
        const char* slotStr,
        const char* data,
        int size);

    //LeoEngine specific commands can be called with this. For example brush change,state change...etc
    //Example: SculptApp_issueAppCmd("SetSculptBrush", "Pull");
    LEOPLUGIN_DLL_API void SculptApp_issueAppCmd(const char* cmd, const char* value);

    //Adds and appcommand specified above to the queue
    LEOPLUGIN_DLL_API void SculptApp_addCmdToQueue(const char* cmd, const char* value);

    //Returns info about the current material
    LEOPLUGIN_DLL_API char* SculptApp_getMaterialInfo();

    //Initializes the LeoEngine
    LEOPLUGIN_DLL_API bool SculptApp_init();

    //Returns whether the mesh is modified since last save
    LEOPLUGIN_DLL_API int SculptMesh_isModified();

    //Shots down the LeoEngine
    LEOPLUGIN_DLL_API void SculptApp_ShutoDown();

    //Executes the the Engine releated commands which have to be used on every frame
    //For example sculpting or painting (with current state of the controllers)...etc
    LEOPLUGIN_DLL_API void SculptApp_Frame();

    //Updates the controller objects' state with the given
    //ControllertIndex
    //DeviceIndex
    //Button states
    //Dpad coords(X,Y)
    //ControllerPose mat
    LEOPLUGIN_DLL_API void setControllerStatesInput(short index, unsigned int deviceIndex, double buttons[4], double dPadX, double dPadY, double poseMat[16]);

    //Sets the camera state with the given
    //PoseMatrix,
    //Eyepose Left
    //EyePose Right
    //Eye Projection matrices
    //Near clipping plane distance
    //Far clipping plane distance
    LEOPLUGIN_DLL_API void setCameraStateInput(double headPoseMat[16], double eyePoseA[16], double eyePoseB[16], double eyeProjA[16], double eyeProjB[16], double nearDist, double farDist);

    //Sets the camera's matrix with the given matrix
    LEOPLUGIN_DLL_API void setCameraMatrix(float matrix[16]);

    //Sets the sensors pose relative to the world with the given matrix
    LEOPLUGIN_DLL_API void setSensorToWorldMat(float matrix[16]);

    //Creates a sculptable mesh for the Leoengine from the given
    //vertices array
    //number of vertices passed
    //indices array
    //number of indices passed
    //normals array
    //number of normals passed
    //Texture coordinates array
    //number of texcoords passed
    //Materials
    //Indices connecting the triangles
    LEOPLUGIN_DLL_API void importFromRawData(float* vertices, unsigned int numVertices, int* indices, unsigned int numIndices, float* normals, unsigned int numNormals, float* texCoords, unsigned int numTexCoords, float worldMat[16], std::vector<IncomingMaterial> metrials,std::vector<unsigned short> triangleMatInds);

    //Returns the number of vertices,indices,normals regarding the current used mesh
    LEOPLUGIN_DLL_API void getSculptMeshNUmberDatas(unsigned int& numVertices, unsigned int& numIndices, unsigned int& numNormals);

    //Return the raw mesh data from LeoEngine
    //vertices array
    //indices array
    //normals array
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