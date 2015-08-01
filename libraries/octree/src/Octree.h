//
//  Octree.h
//  libraries/octree/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Octree_h
#define hifi_Octree_h

#include <set>
#include <SimpleMovingAverage.h>

class CoverageMap;
class ReadBitstreamToTreeParams;
class Octree;
class OctreeElement;
class OctreeElementBag;
class OctreePacketData;
class Shape;


#include "JurisdictionMap.h"
#include "ViewFrustum.h"
#include "OctreeElement.h"
#include "OctreeElementBag.h"
#include "OctreePacketData.h"
#include "OctreeSceneStats.h"

#include <QHash>
#include <QObject>
#include <QReadWriteLock>


extern QVector<QString> PERSIST_EXTENSIONS;

/// derive from this class to use the Octree::recurseTreeWithOperator() method
class RecurseOctreeOperator {
public:
    virtual bool preRecursion(OctreeElement* element) = 0;
    virtual bool postRecursion(OctreeElement* element) = 0;
    virtual OctreeElement* possiblyCreateChildAt(OctreeElement* element, int childIndex) { return NULL; }
};

// Callback function, for recuseTreeWithOperation
typedef bool (*RecurseOctreeOperation)(OctreeElement* element, void* extraData);
typedef enum {GRADIENT, RANDOM, NATURAL} creationMode;
typedef QHash<uint, AACube> CubeList;

const bool NO_EXISTS_BITS         = false;
const bool WANT_EXISTS_BITS       = true;
const bool NO_COLOR               = false;
const bool WANT_COLOR             = true;
const bool COLLAPSE_EMPTY_TREE    = true;
const bool DONT_COLLAPSE          = false;
const bool NO_OCCLUSION_CULLING   = false;
const bool WANT_OCCLUSION_CULLING = true;

const int DONT_CHOP              = 0;
const int NO_BOUNDARY_ADJUST     = 0;
const int LOW_RES_MOVING_ADJUST  = 1;
const quint64 IGNORE_LAST_SENT  = 0;

#define IGNORE_SCENE_STATS       NULL
#define IGNORE_VIEW_FRUSTUM      NULL
#define IGNORE_COVERAGE_MAP      NULL
#define IGNORE_JURISDICTION_MAP  NULL

class EncodeBitstreamParams {
public:
    int maxEncodeLevel;
    int maxLevelReached;
    const ViewFrustum* viewFrustum;
    bool includeColor;
    bool includeExistsBits;
    int chopLevels;
    bool deltaViewFrustum;
    const ViewFrustum* lastViewFrustum;
    bool wantOcclusionCulling;
    int boundaryLevelAdjust;
    float octreeElementSizeScale;
    quint64 lastViewFrustumSent;
    bool forceSendScene;
    OctreeSceneStats* stats;
    CoverageMap* map;
    JurisdictionMap* jurisdictionMap;
    OctreeElementExtraEncodeData* extraEncodeData;

    // output hints from the encode process
    typedef enum {
        UNKNOWN,
        DIDNT_FIT,
        NULL_NODE,
        TOO_DEEP,
        OUT_OF_JURISDICTION,
        LOD_SKIP,
        OUT_OF_VIEW,
        WAS_IN_VIEW,
        NO_CHANGE,
        OCCLUDED
    } reason;
    reason stopReason;

    EncodeBitstreamParams(
        int maxEncodeLevel = INT_MAX,
        const ViewFrustum* viewFrustum = IGNORE_VIEW_FRUSTUM,
        bool includeColor = WANT_COLOR,
        bool includeExistsBits = WANT_EXISTS_BITS,
        int  chopLevels = 0,
        bool deltaViewFrustum = false,
        const ViewFrustum* lastViewFrustum = IGNORE_VIEW_FRUSTUM,
        bool wantOcclusionCulling = NO_OCCLUSION_CULLING,
        CoverageMap* map = IGNORE_COVERAGE_MAP,
        int boundaryLevelAdjust = NO_BOUNDARY_ADJUST,
        float octreeElementSizeScale = DEFAULT_OCTREE_SIZE_SCALE,
        quint64 lastViewFrustumSent = IGNORE_LAST_SENT,
        bool forceSendScene = true,
        OctreeSceneStats* stats = IGNORE_SCENE_STATS,
        JurisdictionMap* jurisdictionMap = IGNORE_JURISDICTION_MAP,
        OctreeElementExtraEncodeData* extraEncodeData = NULL) :
            maxEncodeLevel(maxEncodeLevel),
            maxLevelReached(0),
            viewFrustum(viewFrustum),
            includeColor(includeColor),
            includeExistsBits(includeExistsBits),
            chopLevels(chopLevels),
            deltaViewFrustum(deltaViewFrustum),
            lastViewFrustum(lastViewFrustum),
            wantOcclusionCulling(wantOcclusionCulling),
            boundaryLevelAdjust(boundaryLevelAdjust),
            octreeElementSizeScale(octreeElementSizeScale),
            lastViewFrustumSent(lastViewFrustumSent),
            forceSendScene(forceSendScene),
            stats(stats),
            map(map),
            jurisdictionMap(jurisdictionMap),
            extraEncodeData(extraEncodeData),
            stopReason(UNKNOWN)
    {}

    void displayStopReason() {
        printf("StopReason: ");
        switch (stopReason) {
            default:
            case UNKNOWN: qDebug("UNKNOWN"); break;

            case DIDNT_FIT: qDebug("DIDNT_FIT"); break;
            case NULL_NODE: qDebug("NULL_NODE"); break;
            case TOO_DEEP: qDebug("TOO_DEEP"); break;
            case OUT_OF_JURISDICTION: qDebug("OUT_OF_JURISDICTION"); break;
            case LOD_SKIP: qDebug("LOD_SKIP"); break;
            case OUT_OF_VIEW: qDebug("OUT_OF_VIEW"); break;
            case WAS_IN_VIEW: qDebug("WAS_IN_VIEW"); break;
            case NO_CHANGE: qDebug("NO_CHANGE"); break;
            case OCCLUDED: qDebug("OCCLUDED"); break;
        }
    }

    QString getStopReason() {
        switch (stopReason) {
            default:
            case UNKNOWN: return QString("UNKNOWN"); break;

            case DIDNT_FIT: return QString("DIDNT_FIT"); break;
            case NULL_NODE: return QString("NULL_NODE"); break;
            case TOO_DEEP: return QString("TOO_DEEP"); break;
            case OUT_OF_JURISDICTION: return QString("OUT_OF_JURISDICTION"); break;
            case LOD_SKIP: return QString("LOD_SKIP"); break;
            case OUT_OF_VIEW: return QString("OUT_OF_VIEW"); break;
            case WAS_IN_VIEW: return QString("WAS_IN_VIEW"); break;
            case NO_CHANGE: return QString("NO_CHANGE"); break;
            case OCCLUDED: return QString("OCCLUDED"); break;
        }
    }
};

class ReadElementBufferToTreeArgs {
public:
    const unsigned char* buffer;
    int length;
    bool destructive;
    bool pathChanged;
};

class ReadBitstreamToTreeParams {
public:
    bool includeColor;
    bool includeExistsBits;
    OctreeElement* destinationElement;
    QUuid sourceUUID;
    SharedNodePointer sourceNode;
    bool wantImportProgress;
    PacketVersion bitstreamVersion;
    int elementsPerPacket = 0;
    int entitiesPerPacket = 0;

    ReadBitstreamToTreeParams(
        bool includeColor = WANT_COLOR,
        bool includeExistsBits = WANT_EXISTS_BITS,
        OctreeElement* destinationElement = NULL,
        QUuid sourceUUID = QUuid(),
        SharedNodePointer sourceNode = SharedNodePointer(),
        bool wantImportProgress = false,
        PacketVersion bitstreamVersion = 0) :
            includeColor(includeColor),
            includeExistsBits(includeExistsBits),
            destinationElement(destinationElement),
            sourceUUID(sourceUUID),
            sourceNode(sourceNode),
            wantImportProgress(wantImportProgress),
            bitstreamVersion(bitstreamVersion)
    {}
};

class Octree : public QObject {
    Q_OBJECT
public:
    Octree(bool shouldReaverage = false);
    virtual ~Octree();

    /// Your tree class must implement this to create the correct element type
    virtual OctreeElement* createNewElement(unsigned char * octalCode = NULL) = 0;

    // These methods will allow the OctreeServer to send your tree inbound edit packets of your
    // own definition. Implement these to allow your octree based server to support editing
    virtual bool getWantSVOfileVersions() const { return false; }
    virtual PacketType::Value expectedDataPacketType() const { return PacketType::Unknown; }
    virtual bool canProcessVersion(PacketVersion thisVersion) const {
                    return thisVersion == versionForPacketType(expectedDataPacketType()); }
    virtual PacketVersion expectedVersion() const { return versionForPacketType(expectedDataPacketType()); }
    virtual bool handlesEditPacketType(PacketType::Value packetType) const { return false; }
    virtual int processEditPacketData(NLPacket& packet, const unsigned char* editData, int maxLength,
                                      const SharedNodePointer& sourceNode) { return 0; }
                    
    virtual bool recurseChildrenWithData() const { return true; }
    virtual bool rootElementHasData() const { return false; }
    virtual int minimumRequiredRootDataBytes() const { return 0; }
    virtual bool suppressEmptySubtrees() const { return true; }
    virtual void releaseSceneEncodeData(OctreeElementExtraEncodeData* extraEncodeData) const { }
    virtual bool mustIncludeAllChildData() const { return true; }
    
    /// some versions of the SVO file will include breaks with buffer lengths between each buffer chunk in the SVO
    /// file. If the Octree subclass expects this for this particular version of the file, it should override this
    /// method and return true.
    virtual bool versionHasSVOfileBreaks(PacketVersion thisVersion) const { return false; }

    virtual void update() { } // nothing to do by default

    OctreeElement* getRoot() { return _rootElement; }

    virtual void eraseAllOctreeElements(bool createNewRoot = true);

    void readBitstreamToTree(const unsigned char* bitstream,  unsigned long int bufferSizeBytes, ReadBitstreamToTreeParams& args);
    void deleteOctalCodeFromTree(const unsigned char* codeBuffer, bool collapseEmptyTrees = DONT_COLLAPSE);
    void reaverageOctreeElements(OctreeElement* startElement = NULL);

    void deleteOctreeElementAt(float x, float y, float z, float s);
    
    /// Find the voxel at position x,y,z,s
    /// \return pointer to the OctreeElement or NULL if none at x,y,z,s.
    OctreeElement* getOctreeElementAt(float x, float y, float z, float s) const;
    
    /// Find the voxel at position x,y,z,s
    /// \return pointer to the OctreeElement or to the smallest enclosing parent if none at x,y,z,s.
    OctreeElement* getOctreeEnclosingElementAt(float x, float y, float z, float s) const;
    
    OctreeElement* getOrCreateChildElementAt(float x, float y, float z, float s);
    OctreeElement* getOrCreateChildElementContaining(const AACube& box);

    void recurseTreeWithOperation(RecurseOctreeOperation operation, void* extraData = NULL);
    void recurseTreeWithPostOperation(RecurseOctreeOperation operation, void* extraData = NULL);

    /// \param operation type of operation
    /// \param point point in world-frame (meters)
    /// \param extraData hook for user data to be interpreted by special context
    void recurseTreeWithOperationDistanceSorted(RecurseOctreeOperation operation,
                                                const glm::vec3& point, void* extraData = NULL);

    void recurseTreeWithOperator(RecurseOctreeOperator* operatorObject);

    int encodeTreeBitstream(OctreeElement* element, OctreePacketData* packetData, OctreeElementBag& bag,
                            EncodeBitstreamParams& params) ;
                            
    bool isDirty() const { return _isDirty; }
    void clearDirtyBit() { _isDirty = false; }
    void setDirtyBit() { _isDirty = true; }

    // Octree does not currently handle its own locking, caller must use these to lock/unlock
    void lockForRead() { _lock.lockForRead(); }
    bool tryLockForRead() { return _lock.tryLockForRead(); }
    void lockForWrite() { _lock.lockForWrite(); }
    bool tryLockForWrite() { return _lock.tryLockForWrite(); }
    void unlock() { _lock.unlock(); }
    // output hints from the encode process
    typedef enum {
        Lock,
        TryLock,
        NoLock
    } lockType;

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             OctreeElement*& node, float& distance, BoxFace& face,
                             void** intersectedObject = NULL,
                             Octree::lockType lockType = Octree::TryLock,
                             bool* accurateResult = NULL,
                             bool precisionPicking = false);

    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration, void** penetratedObject = NULL,
                                    Octree::lockType lockType = Octree::TryLock, bool* accurateResult = NULL);

    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration,
                                    Octree::lockType lockType = Octree::TryLock, bool* accurateResult = NULL);

    /// \param cube query cube in world-frame (meters)
    /// \param[out] cubes list of cubes (world-frame) of child elements that have content
    bool findContentInCube(const AACube& cube, CubeList& cubes);

    /// \param point query point in world-frame (meters)
    /// \param lockType how to lock the tree (Lock, TryLock, NoLock)
    /// \param[out] accurateResult pointer to output result, will be set "true" or "false" if non-null
    OctreeElement* getElementEnclosingPoint(const glm::vec3& point,
                                    Octree::lockType lockType = Octree::TryLock, bool* accurateResult = NULL);

    // Note: this assumes the fileFormat is the HIO individual voxels code files
    void loadOctreeFile(const char* fileName, bool wantColorRandomizer);

    // Octree exporters
    void writeToFile(const char* filename, OctreeElement* element = NULL, QString persistAsFileType = "svo");
    void writeToJSONFile(const char* filename, OctreeElement* element = NULL, bool doGzip = false);
    void writeToSVOFile(const char* filename, OctreeElement* element = NULL);
    virtual bool writeToMap(QVariantMap& entityDescription, OctreeElement* element, bool skipDefaultValues) = 0;

    // Octree importers
    bool readFromFile(const char* filename);
    bool readFromURL(const QString& url); // will support file urls as well...
    bool readFromStream(unsigned long streamLength, QDataStream& inputStream);
    bool readSVOFromStream(unsigned long streamLength, QDataStream& inputStream);
    bool readJSONFromStream(unsigned long streamLength, QDataStream& inputStream);
    bool readJSONFromGzippedFile(QString qFileName);
    virtual bool readFromMap(QVariantMap& entityDescription) = 0;

    unsigned long getOctreeElementsCount();

    bool getShouldReaverage() const { return _shouldReaverage; }

    void recurseElementWithOperation(OctreeElement* element, RecurseOctreeOperation operation,
                void* extraData, int recursionCount = 0);

	/// Traverse child nodes of node applying operation in post-fix order
	///
    void recurseElementWithPostOperation(OctreeElement* element, RecurseOctreeOperation operation,
                void* extraData, int recursionCount = 0);

    void recurseElementWithOperationDistanceSorted(OctreeElement* element, RecurseOctreeOperation operation,
                const glm::vec3& point, void* extraData, int recursionCount = 0);

    bool recurseElementWithOperator(OctreeElement* element, RecurseOctreeOperator* operatorObject, int recursionCount = 0);

    bool getIsViewing() const { return _isViewing; } /// This tree is receiving inbound viewer datagrams.
    void setIsViewing(bool isViewing) { _isViewing = isViewing; }

    bool getIsServer() const { return _isServer; } /// Is this a server based tree. Allows guards for certain operations
    void setIsServer(bool isServer) { _isServer = isServer; }

    bool getIsClient() const { return !_isServer; } /// Is this a client based tree. Allows guards for certain operations
    void setIsClient(bool isClient) { _isServer = !isClient; }
    
    virtual void dumpTree() { }
    virtual void pruneTree() { }

    virtual void resetEditStats() { }
    virtual quint64 getAverageDecodeTime() const { return 0; }
    virtual quint64 getAverageLookupTime() const { return 0;  }
    virtual quint64 getAverageUpdateTime() const { return 0;  }
    virtual quint64 getAverageCreateTime() const { return 0;  }
    virtual quint64 getAverageLoggingTime() const { return 0;  }


signals:
    void importSize(float x, float y, float z);
    void importProgress(int progress);

public slots:
    void cancelImport();


protected:
    void deleteOctalCodeFromTreeRecursion(OctreeElement* element, void* extraData);

    int encodeTreeBitstreamRecursion(OctreeElement* element,
                                     OctreePacketData* packetData, OctreeElementBag& bag,
                                     EncodeBitstreamParams& params, int& currentEncodeLevel,
                                     const ViewFrustum::location& parentLocationThisView) const;

    static bool countOctreeElementsOperation(OctreeElement* element, void* extraData);

    OctreeElement* nodeForOctalCode(OctreeElement* ancestorElement, const unsigned char* needleCode, OctreeElement** parentOfFoundElement) const;
    OctreeElement* createMissingElement(OctreeElement* lastParentElement, const unsigned char* codeToReach, int recursionCount = 0);
    int readElementData(OctreeElement *destinationElement, const unsigned char* nodeData,
                int bufferSizeBytes, ReadBitstreamToTreeParams& args);

    OctreeElement* _rootElement;

    bool _isDirty;
    bool _shouldReaverage;
    bool _stopImport;

    QReadWriteLock _lock;
    
    bool _isViewing;
    bool _isServer;
};

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale);

#endif // hifi_Octree_h
