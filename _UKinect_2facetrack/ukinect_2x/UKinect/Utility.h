/*******************************************
*
*	UKinect 
*   Author: Jan Kedzierski
*
********************************************/

#define DEPTH_SCALE_FACTOR (255./65535.)

// Nui skeleton chooser mode
enum ChooserMode
{
    ChooserModeDefault,
    ChooserModeClosest1,
    ChooserModeClosest2,
    ChooserModeSticky1,
    ChooserModeSticky2,
    ChooserModeActive1,
    ChooserModeActive2
};

// Tracked player ID index
enum TrackIDIndex
{
    FirstTrackID = 0,
    SecondTrackID,
    TrackIDIndexCount
};


// Safe release for interfaces
template<class Interface>
inline void SafeRelease( Interface *& pInterfaceToRelease )
{
    if ( pInterfaceToRelease != NULL )
    {
        pInterfaceToRelease->Release();
        pInterfaceToRelease = NULL;
    }
}


class CIneractionClient:public INuiInteractionClient
{
public:
    CIneractionClient()
    {;}
    ~CIneractionClient()
    {;}

    STDMETHOD(GetInteractionInfoAtLocation)(THIS_ DWORD skeletonTrackingId, NUI_HAND_TYPE handType, FLOAT x, FLOAT y, _Out_ NUI_INTERACTION_INFO *pInteractionInfo)
		
    {        
        if(pInteractionInfo)
        {
            pInteractionInfo->IsPressTarget         = false;
            pInteractionInfo->PressTargetControlId  = 0;
            pInteractionInfo->PressAttractionPointX = 0.f;
            pInteractionInfo->PressAttractionPointY = 0.f;
            pInteractionInfo->IsGripTarget          = true;
            return S_OK;
        }
        return E_POINTER;

        //return S_OK; 

    }

    STDMETHODIMP_(ULONG)    AddRef()                                    { return 2;     }
    STDMETHODIMP_(ULONG)    Release()                                   { return 1;     }
    STDMETHODIMP            QueryInterface(REFIID riid, void **ppv)     { return S_OK;  }

};
