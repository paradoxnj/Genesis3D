/****************************************************************************************/
/*  ModelDialog.cpp                                                                     */
/*                                                                                      */
/*  Author:       Jim Mischel, Jeff Lomax                                               */
/*  Description:  Manipulate models in the editor                                       */
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

#include "stdafx.h"
#include "FusionTabControls.h"
#include "ModelDialog.h"
#include "KeyEditDlg.h"
#include "Resource.h"
#include <assert.h>
#include "ram.h"
#include "util.h"

#include "FUSIONDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



enum
{
    KEY_TYPE_INVALID,
    KEY_TYPE_EVENT,
    KEY_TYPE_KEYFRAME
};


struct EventEditType
{
    geFloat Time;
    char EventString[_MAX_PATH];
    char AddOrEdit;
};

/////////////////////////////////////////////////////////////////////////////
// CEventKeyEdit dialog

class CEventKeyEdit : public CDialog
{
// Construction
public:
	CEventKeyEdit(CWnd* pParent, EventEditType *pInfo);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEventKeyEdit)
	enum { IDD = IDD_EVENTEDIT };
	CEdit	m_EventTime;
	CEdit	m_EventString;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventKeyEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    EventEditType *pEditInfo;

	// Generated message map functions
	//{{AFX_MSG(CEventKeyEdit)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};





/////////////////////////////////////////////////////////////////////////////
// CEventKeyEdit dialog


CEventKeyEdit::CEventKeyEdit(CWnd* pParent, EventEditType *pInfo)
	: CDialog(CEventKeyEdit::IDD, pParent), pEditInfo(pInfo)
{
	//{{AFX_DATA_INIT(CEventKeyEdit)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEventKeyEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEventKeyEdit)
	DDX_Control(pDX, IDC_EVENTTIME, m_EventTime);
	DDX_Control(pDX, IDC_EVENTSTRING, m_EventString);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEventKeyEdit, CDialog)
	//{{AFX_MSG_MAP(CEventKeyEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventKeyEdit message handlers
BOOL CEventKeyEdit::OnInitDialog() 
{
    assert (pEditInfo != NULL);

	CDialog::OnInitDialog();
	
    switch (pEditInfo->AddOrEdit)
    {
        case 'A' :
            SetWindowText ("Add event");
            m_EventTime.SetWindowText ("");
            m_EventString.SetWindowText ("");
            break;
        case 'E' :
            char StringTime[20];

            SetWindowText ("Edit event");
            sprintf (StringTime, "%.2f", pEditInfo->Time);
            m_EventTime.SetWindowText (StringTime);
            m_EventString.SetWindowText (pEditInfo->EventString);
            break;
        default :
            assert (0);     // better not happen...
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CEventKeyEdit::OnOK() 
{
    CString EditTime;
    CString EditText;
    float Time;
    
    m_EventTime.GetWindowText (EditTime);
    if (Util_IsValidFloat (EditTime, &Time))
    {
        m_EventString.GetWindowText (EditText);
        if ((EditText.GetLength () == 0) ||
            (EditText.GetLength () >= sizeof (pEditInfo->EventString)))
        {
            m_EventString.SetFocus ();
            m_EventString.SetSel (0, -1, FALSE);
        }
        else
        {
            pEditInfo->Time = Time;
            strcpy (pEditInfo->EventString, EditText);
            CDialog::OnOK();
        }
    }
    else
    {
        m_EventTime.SetFocus ();
        m_EventTime.SetSel (0, -1, FALSE);
    }
}



/////////////////////////////////////////////////////////////////////////////
// CModelDialog dialog


CModelDialog::CModelDialog
	(
	  CWnd* pParent
	) : CDialog(CModelDialog::IDD, pParent),
		pDoc(NULL), pModelInfo(NULL), Animating(GE_FALSE), 
		SettingOrigin(GE_FALSE), EntityIndex (-1)
{
	//{{AFX_DATA_INIT(CModelDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pParentCtrl	= (CFusionTabControls *)pParent;
	geXForm3d_SetIdentity (&XfmDelta);
}

CModelDialog::~CModelDialog 
	(
	  void
	)
{
}

void CModelDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModelDialog)
	DDX_Control(pDX, IDC_SETMODELORIGIN, m_SetModelOrigin);
	DDX_Control(pDX, IDC_CLONEMODEL, m_CloneModel);
	DDX_Control(pDX, IDC_LOCKORIGIN, m_cbLockOrigin);
	DDX_Control(pDX, IDI_STOP, m_Stop);
	DDX_Control(pDX, IDI_RRSTART, m_rrStart);
	DDX_Control(pDX, IDI_RRFRAME, m_rrFrame);
	DDX_Control(pDX, IDI_PLAY, m_Play);
	DDX_Control(pDX, IDI_FFFRAME, m_ffFrame);
	DDX_Control(pDX, IDI_FFEND, m_ffEnd);
	DDX_Control(pDX, IDC_SELECT, m_Select);
	DDX_Control(pDX, IDC_REMOVEBRUSHES, m_RemoveBrushes);
	DDX_Control(pDX, IDC_EDITMODEL, m_EditModel);
	DDX_Control(pDX, IDC_EDITKEY, m_EditKey);
	DDX_Control(pDX, IDC_EDITEVENT, m_EditEvent);
	DDX_Control(pDX, IDC_DESELECT, m_Deselect);
	DDX_Control(pDX, IDC_DELETEMODEL, m_DeleteModel);
	DDX_Control(pDX, IDC_DELETEKEY, m_DeleteKey);
	DDX_Control(pDX, IDC_DELETEEVENT, m_DeleteEvent);
	DDX_Control(pDX, IDC_ADDMODEL, m_AddModel);
	DDX_Control(pDX, IDC_ADDEVENT, m_AddEvent);
	DDX_Control(pDX, IDC_ADDBRUSHES, m_AddBrushes);
	DDX_Control(pDX, IDC_LOCKED, m_cbLocked);
	DDX_Control(pDX, IDC_ANIMATE, m_AnimateButton);
	DDX_Control(pDX, IDC_KEYSLIST, m_KeysList);
	DDX_Control(pDX, IDC_MODELCOMBO, m_ModelCombo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModelDialog, CDialog)
	//{{AFX_MSG_MAP(CModelDialog)
	ON_BN_CLICKED(IDC_ADDMODEL, OnAddmodel)
	ON_BN_CLICKED(IDC_DELETEMODEL, OnDeletemodel)
	ON_BN_CLICKED(IDC_ADDBRUSHES, OnAddbrushes)
	ON_BN_CLICKED(IDC_REMOVEBRUSHES, OnRemovebrushes)
	ON_BN_CLICKED(IDC_SELECT, OnSelectBrushes)
	ON_BN_CLICKED(IDC_DESELECT, OnDeselectBrushes)
	ON_CBN_SELENDOK(IDC_MODELCOMBO, OnSelendokModelcombo)
	ON_BN_CLICKED(IDC_ANIMATE, OnAnimate)
	ON_BN_CLICKED(IDC_DELETEKEY, OnDeletekey)
	ON_LBN_SELCHANGE(IDC_KEYSLIST, OnSelchangeKeyslist)
	ON_BN_CLICKED(IDC_LOCKED, OnLocked)
	ON_BN_CLICKED(IDC_CLONEMODEL, OnClonemodel)
	ON_BN_CLICKED(IDC_SETMODELORIGIN, OnSetmodelorigin)
	ON_BN_CLICKED(IDC_LOCKORIGIN, OnLockorigin)
	ON_BN_CLICKED(IDC_ADDEVENT, OnAddevent)
	ON_BN_CLICKED(IDC_DELETEEVENT, OnDeleteevent)
	ON_BN_CLICKED(IDC_EDITEVENT, OnEditevent)
	ON_BN_CLICKED(IDC_EDITKEY, OnEditkey)
	ON_BN_CLICKED(IDC_EDITMODEL, OnEditmodel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModelDialog message handlers

static const char ModelPrompt[] = 
	"One or more of the selected brushes is already owned by a model.\r"
	"If you continue, those brushes will be removed from their current models\r"
	"and placed in this model.\r\r"
	"Do you want to continue?";


int CModelDialog::GetUniqueModelName
	(
	  CString const &Caption,
	  CString &ModelName
	)
{
	int rslt;
	BOOL NameExists;

	CKeyEditDlg Dlg (this, Caption, &ModelName);

	do
	{
		// Get the model name.
		// It has to be unique.
		rslt = Dlg.DoModal ();
		NameExists = TRUE;
		if (rslt == IDOK)
		{
			if (ModelName == "")
			{
				rslt = AfxMessageBox( IDS_NOBLANKMODEL, MB_OKCANCEL ) ;
			}
			else if (ModelList_FindByName (pModelInfo->Models, ModelName) != NULL)
			{
				CString s;

				s.Format( IDS_MODELEXISTS, ModelName);
				rslt = MessageBox (s, NULL, MB_OKCANCEL);
			}
			else
			{
				NameExists = FALSE;
			}
		}
	} while ((rslt == IDOK) && (NameExists));
	return rslt;
}

void CModelDialog::OnAddmodel() 
{
	CString ModelName;
	int rslt;

	if (SelBrushList_GetSize (pDoc->pSelBrushes) == 0)
	{
		MessageBox ("No brushes selected.", "Add Model", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	// See if any of the selected brushes already belong to a model
	if (SelectedBrushesInOtherModels (0))
	{
		if (MessageBox (ModelPrompt, "Add Model", MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) != IDYES)
		{
			return;
		}
	}

	ModelName = "";
	rslt = GetUniqueModelName ("Enter model name", ModelName);

	if (rslt == IDOK)
	{
		// add the new model to the list.
		// This will set the model id fields in the model's brushes
		if (ModelList_Add (pModelInfo->Models, ModelName, pDoc->pSelBrushes))
		{
			Model *m;

			// get current model and update its object-to-world transform
			m = ModelList_FindByName (pModelInfo->Models, ModelName);
			assert (m != NULL);

			pModelInfo->CurrentModel = Model_GetId (m);

			UpdateModelsList ();
			geXForm3d_SetIdentity (&XfmDelta);
			// we add the first key (identity)
			// (and don't allow it to be edited/deleted)
			AddTheKey (m, 0.0f, &XfmDelta);

			// rebuild trees when brushes added to model.
			pDoc->RebuildTrees ();
		}
		PrivateUpdate ();
	}
}

void CModelDialog::OnDeletemodel() 
// delete the current model.
// This sets the model brushes' model id to 0.
{
	if (pModelInfo->CurrentModel != 0)
	{
		ModelList_Remove (pModelInfo->Models, pModelInfo->CurrentModel, Level_GetBrushes (pDoc->pLevel));
		PrivateUpdate ();
		pDoc->RebuildTrees ();
	}
}

geBoolean CModelDialog::SelectedBrushesInOtherModels
	(
	  int ModelId
	)
{
	int iBrush;
	int NumSelBrushes;

	// First check to see if any of the selected brushes belong to some other model.
	// If they do, then prompt the user.
	NumSelBrushes = SelBrushList_GetSize (pDoc->pSelBrushes);
	for (iBrush = 0; iBrush < NumSelBrushes; iBrush++)
	{
		Brush *pBrush;
		int BrushModelId;

		pBrush = SelBrushList_GetBrush (pDoc->pSelBrushes, iBrush);
		BrushModelId = Brush_GetModelId (pBrush);
		if ((BrushModelId != 0) && (BrushModelId != ModelId))
		{
			return GE_TRUE;
		}
	}
	return GE_FALSE;
}

void CModelDialog::OnAddbrushes() 
// Add selected brushes to this model
{
	Model *pModel;
	int ModelId;
	BOOL AddBrushes;

	pModel = GetCurrentModel ();
	if (pModel == NULL)
	{
		MessageBox ("No brushes selected.", "Add model brushes", MB_OK);
		return;
	}
	
	AddBrushes = TRUE;
	ModelId = Model_GetId (pModel);
	if (SelectedBrushesInOtherModels (ModelId))
	{
		// prompt user
		int rslt;

		rslt = MessageBox (ModelPrompt, "Add model brushes", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
		if (rslt != IDYES)
		{
			AddBrushes = FALSE;
		}
	}		

	if (AddBrushes)
	{
		Model_AddBrushes (pModel, pDoc->pSelBrushes);

		// make sure that all model brushes are selected if this is a locked model...
		Brush *pBrush = SelBrushList_GetBrush (pDoc->pSelBrushes, 0);

		pDoc->DoBrushSelection (pBrush, brushSelAlways);

		// Have to rebuild the trees when brushes are added to a model.
		pDoc->RebuildTrees ();
		pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);
	}
}

void CModelDialog::OnRemovebrushes() 
// remove selected brushes from this model
{
	Model *pModel;

	pModel = GetCurrentModel ();
	if (pModel != NULL)
	{
		Model_RemoveBrushes (pModel, pDoc->pSelBrushes);
		pDoc->RebuildTrees ();
	}
}

void CModelDialog::OnSelectBrushes() 
// Select all of the brushes that are in the current model
// This will deselect any other current selection??
{
	Model *pModel;

	pModel = GetCurrentModel ();
	if (pModel != NULL)
	{
		int ModelId;

		ModelId = Model_GetId (pModel);

		pDoc->DoGeneralSelect ();
		pDoc->ResetAllSelections ();

		pDoc->SelectModelBrushes (TRUE, ModelId);

		pDoc->mCurrentTool=ID_TOOLS_BRUSH_MOVEROTATEBRUSH;
		pDoc->ConfigureCurrentTool();
		
		pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);
	}
}


void CModelDialog::OnDeselectBrushes() 
// Deselect all brushes that belong to the current model.
{
	Model *pModel;

	pModel = GetCurrentModel ();
	if (pModel != NULL)
	{
		int CurrentModelId;

		CurrentModelId = Model_GetId (pModel);

		pDoc->SelectModelBrushes (FALSE, CurrentModelId);
		pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
	}
}

void CModelDialog::PostNcDestroy() 
{
	CDialog::PostNcDestroy();
	/*
	  Modeless dialog boxes delete themselves.
	  (quiver, spasm, twitch)
	*/
//	delete this;
}

void CModelDialog::Update
	(
	  CFusionDoc *aDoc,
	  ModelInfo_Type *aModelInfo
	)
{
//	if (pDoc != aDoc)
	{
		pDoc = aDoc;
		pModelInfo = aModelInfo;

		PrivateUpdate ();
	}
}

void CModelDialog::PrivateUpdate
	(
	  void
	)
{
	if (pDoc == NULL)
	{
		EnableControls (FALSE, NULL);
	}
	else
	{
		Model *pModel;

		pModel = GetCurrentModel ();
		UpdateModelsList ();
		UpdateKeysList (pModel);
	}
}

void CModelDialog::UpdateModelsList
	(
	  void
	)
// updates the list of models in the combo box
// Puts the model's name in the text field, and the model id
// in SetItemData.
{
	ModelIterator mi;
	int Index;
	Model *pModel;

	m_ModelCombo.ResetContent ();

	// add no-selection item.
	// This one can't be deleted and should always be first!
	Index = m_ModelCombo.AddString ("<none>");
	if (Index != CB_ERR)
	{
		m_ModelCombo.SetItemData (Index, 0);
		m_ModelCombo.SetCurSel (Index);
	}

	pModel = ModelList_GetFirst (pModelInfo->Models, &mi);
	while (pModel != NULL)
	{
		int ModelId;
		char const * ModelName;

		ModelName = Model_GetName (pModel);
		ModelId = Model_GetId (pModel);
		Index = m_ModelCombo.AddString (ModelName);
		m_ModelCombo.SetItemData (Index, ModelId);

		if (ModelId == pModelInfo->CurrentModel)
		{
			m_ModelCombo.SetCurSel (Index);
		}

		pModel = ModelList_GetNext (pModelInfo->Models, &mi);
	}
}

void CModelDialog::SetListboxKeySelection
	(
	  geFloat KeyTime
	)
// Set listbox current position to first key that's >= passed key time.
{
	int KeyNo;

	for (KeyNo = 0; (KeyNo < m_KeysList.GetCount ()); ++KeyNo)
	{
		CString lbText;

		m_KeysList.GetText (KeyNo, lbText);
		// only interested in keys right now...
		if (lbText[0] == 'K')
		{
			geFloat Time;
			LONG lbData;

			lbData = m_KeysList.GetItemData (KeyNo);
			Time = *((geFloat *)&lbData);

			if (Time >= KeyTime)
			{
				break;
			}
		}
	}

	if (KeyNo >= m_KeysList.GetCount ())
	{
		KeyNo = m_KeysList.GetCount () - 1;
	}
	m_KeysList.SetCurSel (KeyNo);
}


struct KeyEventEntry
{
	geFloat Time;
	char *pString;
};

static int __cdecl KeyEventCompare
	(
	  void const *pvKey1,
	  void const *pvKey2
	)
{
	KeyEventEntry const *pKey1 = (KeyEventEntry const *)pvKey1;
	KeyEventEntry const *pKey2 = (KeyEventEntry const *)pvKey2;
	
	if (pKey1->Time < pKey2->Time) return -1;
	if (pKey1->Time > pKey2->Time) return 1;
	return 0;		
}

void CModelDialog::UpdateKeysList
	(
	  Model *pModel
	)
// Displays the current model's key frames and events in the list box.
{
	gePath *pPath;
	geMotion *pMotion;
	KeyEventEntry *pKeyEventsList;
	int KeyEventCount;

	m_KeysList.ResetContent ();

	if (pModel == NULL)
	{
		EnableControls (FALSE, NULL);
		m_AddModel.EnableWindow (TRUE);
		m_ModelCombo.EnableWindow (TRUE);
		return;
	}

	if (!Animating)
	{
		EnableControls (TRUE, pModel);
	}

	// set state of Locked check...
	m_cbLocked.SetCheck (Model_IsLocked (pModel) ? 1 : 0);
	m_cbLockOrigin.SetCheck (Model_IsRotationLocked (pModel) ? 1 : 0);
	m_cbLockOrigin.EnableWindow (Model_GetNumKeys (pModel) > 1);

#pragma message ("Clean this up and move path/motion twiddling to Model.c")
	pPath = Model_GetPath (pModel);
	pMotion = Model_GetMotion (pModel);

	assert (pPath != NULL);		// can't happen
	assert (pMotion != NULL);	// really!

	pKeyEventsList = NULL;
	// count number of keyframes and events and allocate
	// array of KeyEventEntry structures...
	{
		int EventCount;
		geFloat StartTime, EndTime;
		geFloat EventTime;
		char const *EventString;
		int KeyframeCount;

		KeyframeCount = gePath_GetKeyframeCount (pPath, GE_PATH_ROTATION_CHANNEL);
		EventCount = 0;

		/*
		  geMotion_GetEventExtents will crash if there are no events.
		  Which means that it's a useless function.
		  So we'll set the event extents to -10000..10000.  This limits our
		  events range to +/- 10,000 seconds, or about 2 hours and 45 minutes.
		*/
		StartTime = -10000.0f;
		EndTime = 10000.0f;
//		if (geMotion_GetEventExtents (pMotion, &StartTime, &EndTime) != GE_FALSE)
		{
			EndTime += 1.0f;

			geMotion_SetupEventIterator (pMotion, StartTime, EndTime);
			while (geMotion_GetNextEvent (pMotion, &EventTime, &EventString) != GE_FALSE)
			{
				++EventCount;
			}
		}
		KeyEventCount = EventCount + KeyframeCount;

		// allocate the events buffer
		if (KeyEventCount > 0)
		{
			pKeyEventsList = (KeyEventEntry *)geRam_Allocate(sizeof (KeyEventEntry) * KeyEventCount);
		}

		// fill the buffer
		if (pKeyEventsList != NULL)
		{
			int ListItemNo;
			KeyEventEntry *pEntry;
			int i;

			ListItemNo = 0;

			// first the keyframes
			for (i = 0; i < KeyframeCount; ++i)
			{
				geFloat Time;
				geXForm3d Xfm;
				char Text[100];

				// Xfm isn't used here, just time for display
				gePath_GetKeyframe (pPath, i, GE_PATH_ROTATION_CHANNEL, &Time, &Xfm);

				sprintf (Text, "K %.2f", (float)Time);
				pEntry = &pKeyEventsList[ListItemNo];
				pEntry->Time = Time;
				pEntry->pString = Util_Strdup (Text);
				++ListItemNo;
			}

			// and then the events
			if (EventCount > 0)
			{
				geMotion_SetupEventIterator (pMotion, StartTime, EndTime);
				while (geMotion_GetNextEvent (pMotion, &EventTime, &EventString) != GE_FALSE)
				{
					int len;
					char Text[20];
										
					sprintf (Text, "E %.2f ()", (float)EventTime);
					if (EventString == NULL)
					{
						EventString = "";
					}
					len = strlen (Text) + strlen (EventString) + 1;
					pEntry = &pKeyEventsList[ListItemNo];
					pEntry->Time = EventTime;
					pEntry->pString = (char *)geRam_Allocate(len);
					if (pEntry->pString != NULL)
					{
						sprintf (pEntry->pString, "E %.2f (%s)", (float)EventTime, EventString);
					}
					++ListItemNo;
				}
			}
			assert (ListItemNo == KeyEventCount);

			// sort the list by time...
			qsort (pKeyEventsList, KeyEventCount, sizeof (KeyEventEntry), KeyEventCompare);

			// fill up the listbox
			for (i = 0; i < KeyEventCount; ++i)
			{
				int Index;
				
				pEntry = &pKeyEventsList[i];
				Index = m_KeysList.AddString (pEntry->pString);
				if (Index != LB_ERR)
				{
					// Yes, that's an ugly cast.  I want to pass the value as though it
					// is a LONG, but I don't want to do any conversion...
					m_KeysList.SetItemData (Index, *((LONG*)&(pEntry->Time)));
				}
			}

			// and, finally, destroy the list
			for (i = 0; i < KeyEventCount; ++i)
			{
				pEntry = &pKeyEventsList[i];

				if (pEntry->pString != NULL)
				{
					geRam_Free (pEntry->pString);
					pEntry->pString = NULL;
				}
			}
			geRam_Free (pKeyEventsList);
		}
	}


	SetListboxKeySelection (Model_GetCurrentKeyTime (pModel));
}

void CModelDialog::OnSelendokModelcombo() 
{
	if (pDoc != NULL)
	{
		int Index;

		Index = m_ModelCombo.GetCurSel ();
		if (Index == CB_ERR)
		{
			pModelInfo->CurrentModel = 0;
		}
		else
		{
			pModelInfo->CurrentModel = m_ModelCombo.GetItemData (Index);
		}
	}
	UpdateKeysList (GetCurrentModel ());
}

void CModelDialog::OnOK() 
{
	// it's a modeless dialog.  Don't allow OK.
}

void CModelDialog::OnCancel() 
{
	if (Animating)
	{
		// can't close if we're animating...
		::MessageBeep (MB_ICONEXCLAMATION);
		return;
	}
}

Model *CModelDialog::GetCurrentModel
	(
	  void
	)
{
	int CurSel;
	int CurrentModelId;

	
	CurSel = m_ModelCombo.GetCurSel ();
	if (CurSel == CB_ERR)
	{
		// no valid model selected
		return NULL;
	}
	CurrentModelId = m_ModelCombo.GetItemData (CurSel);
	if (CurrentModelId == 0)
	{
		// 0 is not a valid model number
		return NULL;
	}
	// make sure model number is valid...
	assert (CurrentModelId > 0);
	
	return ModelList_FindById (pModelInfo->Models, CurrentModelId);
}	

void CModelDialog::AddTheKey
	(
	  Model *pModel,
	  geFloat Time,
	  geXForm3d const *pXfm
	)
// input pXfm must be model-relative!
{
	Model_AddKeyframe (pModel, Time, pXfm);
	UpdateKeysList (pModel);
}

void CModelDialog::AddKey() 
{
	/*
	  User requested to add a key.

	  o Prompt the user for the motion time.
	  o Compute the transformation matrix using the model's reference transform.
	  o Add the key to the model's motion.
	  o Update list box and focus new item.
	*/
	float Time;
	Model *pModel;
	CString const Prompt = "Enter key time";
	CString Value;
	CFloatKeyEditDlg *pDlg;
	int SaveNumKeys;
	geBoolean GotKeyTime;
	geXForm3d XfmCurrent;
	
	pModel = GetCurrentModel ();

	// shouldn't be able to get here w/o a selected model!
	assert (pModel != NULL);

	SaveNumKeys = Model_GetNumKeys (pModel);	// need to know this...
	assert (SaveNumKeys > 0);	// I think this is required...

	// get the current key.
	// We have to do this here because the key might be deleted before we're
	// ready to muck with the current transform.
	Model_GetKeyframe (pModel, Model_GetCurrentKeyTime (pModel), &XfmCurrent);

	GotKeyTime = GE_FALSE;
	do
	{
		int rslt;
		geXForm3d XfmTemp;
	
		// prompt user for time
		pDlg = new CFloatKeyEditDlg (this, Prompt, &Value);
		rslt = pDlg->DoModal ();
		delete pDlg;

		if (rslt != IDOK)
		{
			// user cancelled
			ReverseDeltas (pModel);

			// and update the views...
			pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
			return;
		}
		sscanf (Value, "%f", &Time);

		if (Time <= 0.0f)
		{
			rslt = MessageBox ("Key time must be greater than 0.", "Add key", MB_ICONINFORMATION | MB_OKCANCEL);
			if (rslt == IDCANCEL)
			{
				ReverseDeltas (pModel);
				pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);
				return;
			}
		}
		else if (Model_GetKeyframe (pModel, Time, &XfmTemp))
		// if a key already exists at this time, then we need to prompt user.
		{
			CString msg;

			msg.Format ("A key with time %s already exists.\rDo you want to replace it?", Value);
			
			rslt = MessageBox (msg, "Add key", MB_ICONQUESTION | MB_YESNOCANCEL);
			switch (rslt)
			{
				case IDCANCEL :
				{
					ReverseDeltas (pModel);
					pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);
					return;
				}
				case IDNO :
					break;
				case IDYES :
					GotKeyTime = GE_TRUE;
					Model_DeleteKeyframe (pModel, Time);
					break;
			}
		}
		else
		{
			GotKeyTime = GE_TRUE;
		}

	} while (!GotKeyTime);

	{
		// apply deltas (world-relative in XfmDelta)
		// to current object-relative transform.
		geVec3d VecXlate;

		// back out the translations
		VecXlate = XfmCurrent.Translation;
		geXForm3d_Translate (&XfmCurrent, -VecXlate.X, -VecXlate.Y, -VecXlate.Z);

		// do the rotation
		geXForm3d_Multiply (&XfmDelta, &XfmCurrent, &XfmCurrent);

		// and re-apply the translations
		geXForm3d_Translate (&XfmCurrent, VecXlate.X, VecXlate.Y, VecXlate.Z);

		AddTheKey (pModel, Time, &XfmCurrent);

		// deltas are again 0...
		geXForm3d_SetIdentity (&XfmDelta);
	}

	if ((SaveNumKeys == 1) && (Model_GetNumKeys (pModel) > 1))
	{
		// there was only one key, and now there are multiple keys.
		// Which means that the RotationLock was disabled and the
		// checkbox was disabled.  Enable them.
		Model_SetRotationLock (pModel, GE_TRUE);
		m_cbLockOrigin.SetCheck (1);
		m_cbLockOrigin.EnableWindow (TRUE);
	}

	SetListboxKeySelection (Time);
	Model_SetCurrentKeyTime (pModel, Time);
}

void CModelDialog::GetTranslation (geVec3d *pVec)
{
	*pVec = XfmDelta.Translation;
}


void CModelDialog::UpdateSelectedModel 
	(
	  int MoveRotate, 
	  geVec3d const *pVecDelta
	)
{
	Model *pModel;

	pModel = GetCurrentModel ();
	if (Animating && (pModel != NULL))
	{
		geXForm3d *pXfm;

		// apply translation/rotation to model's delta position
		pXfm = &XfmDelta;

		switch (MoveRotate)
		{
			case BRUSH_MOVE :
			{
				// simple translation
				geXForm3d_Translate (pXfm, pVecDelta->X, pVecDelta->Y, pVecDelta->Z);
				break;
			}
			case BRUSH_ROTATE :
			{
				geXForm3d XfmRotate;

				// build rotation transform from angles
				geXForm3d_SetEulerAngles (&XfmRotate, pVecDelta);

				// and apply it
				{
					/*
					  The problem here is that this new rotation has to be
					  applied AFTER the existing rotations.  So I need to
					  back out the current translations, apply this rotation,
					  and then re-apply the translations.
					*/
					geVec3d VecXlate;

					VecXlate = pXfm->Translation;
					geVec3d_Clear (&(pXfm->Translation));

					geXForm3d_Multiply (&XfmRotate, pXfm, pXfm);
					pXfm->Translation = VecXlate;
				}

				break;
			}
			default:
				// what did you want to do?
				assert (0);
				break;
		}
	}
}

void CModelDialog::EnableControls
	(
	  BOOL Enable,
	  Model const *pModel
	)
// This function called to enable/disable dialog controls
{
	#pragma message ("VCR control buttons disabled")
    // Permamently disable the VCR control buttons...
	m_Stop.EnableWindow (FALSE);
	m_rrStart.EnableWindow (FALSE);
	m_rrFrame.EnableWindow (FALSE);
	m_Play.EnableWindow (FALSE);
	m_ffFrame.EnableWindow (FALSE);
	m_ffEnd.EnableWindow (FALSE);

	m_Select.EnableWindow (Enable);
	m_RemoveBrushes.EnableWindow (Enable);
	m_AddModel.EnableWindow (Enable);

	#pragma message ("Edit Model button disabled")
	m_EditModel.EnableWindow (FALSE);

	m_DeleteModel.EnableWindow (Enable);

	m_EditKey.EnableWindow (Enable);

	m_DeleteKey.EnableWindow (Enable);
	m_AddEvent.EnableWindow (Enable);

	#pragma message ("Edit Event button disabled")
	m_EditEvent.EnableWindow (FALSE);

	m_DeleteEvent.EnableWindow (Enable);
	m_Deselect.EnableWindow (Enable);
	m_AddBrushes.EnableWindow (Enable);
	m_cbLocked.EnableWindow (Enable);
	m_KeysList.EnableWindow (Enable);
	m_ModelCombo.EnableWindow (Enable);
	m_CloneModel.EnableWindow (Enable);
	m_SetModelOrigin.EnableWindow (Enable);
	m_AnimateButton.EnableWindow (Enable);

	if ((pModel == NULL) || !Enable)
	{
		m_cbLockOrigin.EnableWindow (FALSE);
	}
	else
	{
		m_cbLockOrigin.EnableWindow ((Model_GetNumKeys (pModel) > 1));
	}

    if ((pModel != NULL) && Enable)
    {
        int CurSel;
        int KeyType;
        geFloat Time;

    	CurSel = GetCurrentLbKey (&Time, &KeyType);

	    m_EditKey.EnableWindow ((KeyType == KEY_TYPE_KEYFRAME));
#pragma message ("Editing events disabled")
//	    m_EditEvent.EnableWindow ((KeyType == KEY_TYPE_EVENT));
	    m_DeleteKey.EnableWindow ((KeyType == KEY_TYPE_KEYFRAME));
	    m_DeleteEvent.EnableWindow ((KeyType == KEY_TYPE_EVENT));
    }
}

void CModelDialog::OnAnimate() 
{
	Model *pModel;

	pModel = GetCurrentModel ();

	if (Animating)
	{
		assert (pModel != NULL);  // can't happen (right?)
		// Stop animating and add key at this position
		Model_SetAnimating (pModel, GE_FALSE);
		Animating = GE_FALSE;
		EnableControls (TRUE, pModel);
		m_AnimateButton.SetWindowText ("&Animate");
		m_AnimateButton.EnableWindow (TRUE);
		AddKey ();
	}
	else
	{
		if (pModel != NULL)
		{
			// start animating
			Animating = GE_TRUE;
			Model_SetAnimating (pModel, GE_TRUE);
			EnableControls (FALSE, pModel);
			geXForm3d_SetIdentity (&XfmDelta);
			m_AnimateButton.SetWindowText ("Stop &Animating");
			m_AnimateButton.EnableWindow (TRUE);

			OnSelectBrushes ();  // selects the model and all of its brushes
		}
	}
}

int CModelDialog::GetCurrentLbKey
	(
	  geFloat *pTime,
      int *pKeyType
	)
// Returns the key number of the current listbox selection
// Returns 0 if no keys.
{
	int CurSel;
	int KeyNo;
    int EventNo;
    int retval;
	CString lbText;
	LONG lbData;
    int KeyType;
		
	assert (pTime != NULL);
    assert (pKeyType != NULL);

	CurSel = m_KeysList.GetCurSel ();
	
	if (CurSel == LB_ERR)
	{
		return 0;
	}

	m_KeysList.GetText (CurSel, lbText);

	lbData = m_KeysList.GetItemData (CurSel);
	// ugly cast.
	// The item returned is a float that's stored in a long.
	*pTime = *((geFloat *)&lbData);

	// now iterate the listbox items counting the keys until we get here...
	KeyNo = 0;
    EventNo = 0;
    retval = 0;
    KeyType = KEY_TYPE_INVALID;
	for (int i = 0; i < m_KeysList.GetCount (); ++i)
	{
		m_KeysList.GetText (i, lbText);
        switch (lbText[0])
        {
            case 'E' :
                retval = EventNo;
                KeyType = KEY_TYPE_EVENT;
                ++EventNo;
                break;
            case 'K' :
                KeyType = KEY_TYPE_KEYFRAME;
                retval = KeyNo;
                ++KeyNo;
                break;
            default :
                assert (0);     // something bad in listbox
        }
        if (i == CurSel)
        {
            *pKeyType = KeyType;
            return retval;
        }
	}

    assert (0);     // current selection bigger than key count?
	return -1;
}

void CModelDialog::ReverseDeltas
	(
	  Model *pModel
	)
{
	// This code figures out where we are, then calls Model_TransformFromTo
	// to put us back to the current key.
	geVec3d VecXlate;
	geXForm3d XfmCurrent;
	geXForm3d XfmWork;

	assert (pModel != NULL);

	// get the current key
	Model_GetKeyframe (pModel, Model_GetCurrentKeyTime (pModel), &XfmCurrent);

	// XfmWork is used to calculate current position.
	XfmWork = XfmCurrent;

	// back out the translations
	VecXlate = XfmWork.Translation;
	geXForm3d_Translate (&XfmWork, -VecXlate.X, -VecXlate.Y, -VecXlate.Z);

	// do the rotation
	geXForm3d_Multiply (&XfmDelta, &XfmWork, &XfmWork);

	// and re-apply the translations
	geXForm3d_Translate (&XfmWork, VecXlate.X, VecXlate.Y, VecXlate.Z);

	Model_TransformFromTo (pModel, &XfmWork, &XfmCurrent, Level_GetBrushes (pDoc->pLevel));

	// deltas are cleared!
	geXForm3d_SetIdentity (&XfmDelta);
}

void CModelDialog::OnDeletekey() 
// delete the selected key
{
	Model *pModel;

	pModel = GetCurrentModel ();
	if (pModel != NULL)
	{
		int CurSel;
		geFloat Time;
		int KeyType;

		CurSel = GetCurrentLbKey (&Time, &KeyType);

		if ((CurSel != LB_ERR) && (KeyType == KEY_TYPE_KEYFRAME) && (Time != 0.0f))  // can't delete first key!!
		{
			geXForm3d XfmFrom, XfmTo;

			geXForm3d_SetIdentity (&XfmFrom);
			geXForm3d_SetIdentity (&XfmTo);

			Time = Model_GetCurrentKeyTime (pModel);

			// get current transform
			Model_GetKeyframe (pModel, Time, &XfmFrom);

			ReverseDeltas (pModel);

			// position to key 0
			Model_GetKeyframe (pModel, 0.0f, &XfmTo);
			Model_TransformFromTo (pModel, &XfmFrom, &XfmTo, Level_GetBrushes (pDoc->pLevel));

			Model_SetCurrentKeyTime (pModel, 0.0f);

			// remove this keyframe from the model
			Model_DeleteKeyframe (pModel, Time);

			// and from the listbox
//			m_KeysList.DeleteString (CurSel);

			// and finally update the views
			pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);

			if (Model_GetNumKeys (pModel) == 1)
			{
				Model_SetRotationLock (pModel, FALSE);
				m_cbLockOrigin.SetCheck (0);
				m_cbLockOrigin.EnableWindow (FALSE);
			}
			UpdateKeysList (pModel);
		}
	}
}

void CModelDialog::OnSelchangeKeyslist() 
{
	// selection has changed.
	// Move to the new selection
	Model *pModel;
	int CurSel;
	geFloat Time;
    int KeyType;

	pModel = GetCurrentModel ();
	assert (pModel != NULL);	// MUST exist if we get here

	CurSel = GetCurrentLbKey (&Time, &KeyType);
    switch (KeyType)
    {
        case KEY_TYPE_EVENT :
	        m_EditKey.EnableWindow (FALSE);
#pragma message ("Editing events disabled")
	        m_EditEvent.EnableWindow (FALSE);
//	        m_EditEvent.EnableWindow (TRUE);
	        m_DeleteKey.EnableWindow (FALSE);
	        m_DeleteEvent.EnableWindow (TRUE);
            break;

        case KEY_TYPE_KEYFRAME :
        {
	        geXForm3d XfmFrom;
	        geXForm3d XfmTo;

	        ReverseDeltas (pModel);

	        // current position
	        Model_GetKeyframe (pModel, Model_GetCurrentKeyTime (pModel), &XfmFrom);

	        // get transform to new position
	        Model_GetKeyframe (pModel, Time, &XfmTo);

	        // and transform to new position
	        Model_TransformFromTo (pModel, &XfmFrom, &XfmTo, Level_GetBrushes (pDoc->pLevel));

	        // set model's new current keyframe
	        Model_SetCurrentKeyTime (pModel, Time);
	        pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);

	        m_EditKey.EnableWindow (TRUE);
	        m_DeleteKey.EnableWindow (TRUE);
	        m_EditEvent.EnableWindow (FALSE);
	        m_DeleteEvent.EnableWindow (FALSE);

            break;
        }

        default :
            assert (0);     // bad key type returned
    }
}

void CModelDialog::OnLocked() 
{
	int Locked;
	Model *pModel;

	Locked = m_cbLocked.GetCheck ();
	pModel = GetCurrentModel ();
	if (pModel != NULL)
	{
		Model_SetLock (pModel, (Locked == 1));
	}
}


void CModelDialog::OnClonemodel() 
{
	CString ModelName;
	Model *pModel;
	char const *pName;
	int CopyNum;
	CString tag;
	BrushList *BList = Level_GetBrushes (pDoc->pLevel);

	pModel = GetCurrentModel ();
	assert (pModel != NULL);

	// Create default unique name.
	pName = Model_GetName (pModel);
	if (pName == NULL)
	{
		pName = "Model";
	}

	CopyNum = 0;
	do
	{
		++CopyNum;
		ModelName.Format ("%s_Copy%d", pName, CopyNum);
	} while (ModelList_FindByName (pModelInfo->Models, ModelName) != NULL);

	// Prompt user for name.  If user cancels, then exit.
	if (GetUniqueModelName ("Enter name for new model", ModelName) != IDOK)
	{
		return;
	}

	{
		// Make list of cloned brushes.
		BrushList *pNewBrushes;
		Brush *pBrush;
		BrushIterator bi;
		Model *pNewModel;
		int ModelId;


		pNewBrushes = BrushList_Create ();
		if (pNewBrushes == NULL)
		{
			MessageBox ("Out of memory.", "Clone model.", MB_OK | MB_ICONEXCLAMATION);
			return;
		}

		// clone model brushes
		ModelId = Model_GetId (pModel);
		pBrush = BrushList_GetFirst (BList, &bi);
		while (pBrush != NULL)
		{
			if (Brush_GetModelId (pBrush) == ModelId)
			{
				Brush *pNewBrush;

				pNewBrush = Brush_Clone (pBrush);
				BrushList_Append (pNewBrushes, pNewBrush);
			}
			pBrush = BrushList_GetNext (&bi);
		}

		pNewModel = ModelList_AddFromBrushList (pModelInfo->Models, ModelName, pNewBrushes);
		if (pNewModel != NULL)
		{
			int Index;
			int NewModelId;

			// add brushes to doc's brush list (remove from temporary list)
			pBrush = BrushList_GetFirst (pNewBrushes, &bi);
			while (pBrush != NULL)
			{
				// remove from temporary list
				BrushList_Remove (pNewBrushes, pBrush);
				// add to doc's list
				BrushList_Append (BList, pBrush);

				pBrush = BrushList_GetNext (&bi);
			}

			// Copy model keys and other stuff
			Model_CopyStuff (pNewModel, pModel);
			
			// Add new model to combo box
			NewModelId = Model_GetId (pNewModel);
			Index = m_ModelCombo.AddString (ModelName);
			m_ModelCombo.SetItemData (Index, NewModelId);
			m_ModelCombo.SetCurSel (Index);

			pModelInfo->CurrentModel =  NewModelId;
			// Select new model
			OnSelectBrushes ();

			// now move it just a bit to offset from the one we cloned
			{
				geXForm3d XfmMove;
				// offset in X and Z only per artist request
				geXForm3d_SetTranslation (&XfmMove, 100.0f, 0.0f, 100.0f);
				Model_UpdateOrigin (pNewModel, BRUSH_MOVE, &(XfmMove.Translation));
				Model_Transform (pNewModel, &XfmMove, BList);
			}
			pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);
		}
		else
		{
			MessageBox ("Error cloning model.", "Clone model.", MB_OK | MB_ICONEXCLAMATION);
		}

		if (pNewBrushes != NULL)
		{
			BrushList_Destroy (&pNewBrushes);
		}
	}
	
}

void CModelDialog::OnSetmodelorigin() 
{
	Model *pModel;

	pModel = GetCurrentModel ();
	if (SettingOrigin)
	{
		assert (pModel != NULL);
		// Update model's reference transform to take new origin into account.
		StopSettingOrigin (pModel);

		// update UI to reflect new state
		m_SetModelOrigin.SetWindowText ("Set &Origin");
		SettingOrigin = GE_FALSE;
		Animating = GE_FALSE;
		Model_SetAnimating (pModel, GE_FALSE);
		EnableControls (GE_TRUE, pModel);
	}
	else
	{
		if (pModel != NULL)
		{
			if (StartSettingOrigin (pModel))
			{
				Model_SetAnimating (pModel, GE_TRUE);
				Animating = GE_TRUE;
				SettingOrigin = GE_TRUE;
				EnableControls (GE_FALSE, pModel);
				m_SetModelOrigin.SetWindowText ("D&one");
				m_SetModelOrigin.EnableWindow (TRUE);
			}
		}
	}
}

geBoolean CModelDialog::StartSettingOrigin
	(
	  Model *pModel
	)
/*
  Here I insert a special ModelOrigin entity and select it.
  Then allow user to move it as desired.  When done, press the Done button.
*/
// Create ModelOrigin entity and place it at the model's current origin
{
	geBoolean rslt;
	CEntity Ent;

	assert (pModel != NULL);
	assert (EntityIndex == -1);

	rslt = GE_FALSE;

	if (pDoc->CreateEntityFromName ("ModelOrigin", Ent))
	{
		geVec3d org;
		CEntityArray *Entities;

		Model_GetCurrentPos (pModel, &org);

		Ent.SetOrigin (org.X, org.Y, org.Z, Level_GetEntityDefs (pDoc->pLevel));
		EntityIndex = Level_AddEntity (pDoc->pLevel, Ent);

		pDoc->DoGeneralSelect ();
		pDoc->ResetAllSelections ();

		Entities = Level_GetEntities (pDoc->pLevel);
		pDoc->SelectEntity (&(*Entities)[EntityIndex]);

		pDoc->mCurrentTool=ID_TOOLS_BRUSH_MOVEROTATEBRUSH;
		pDoc->ConfigureCurrentTool();
		
		pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);

		rslt = GE_TRUE;
	}

	return rslt;
}

void CModelDialog::StopSettingOrigin
	(
	  Model *pModel
	)
{
	geVec3d org, CurPos, VecDelta;
	CEntity *pEnt;
	CEntityArray *Entities = Level_GetEntities (pDoc->pLevel);

	assert (pModel != NULL);
	assert (EntityIndex != -1);

	// reset the deltas...
	geXForm3d_SetIdentity (&XfmDelta);

	// get entity's position
	pEnt = &(*Entities)[EntityIndex];
	org = pEnt->mOrigin;

	// get model's current position
	Model_GetCurrentPos (pModel, &CurPos);

	// compute deltas...
	geVec3d_Subtract (&org, &CurPos, &VecDelta);
	
	// and move the model's origin (but not the brushes!)
	Model_UpdateOrigin (pModel, BRUSH_MOVE, &VecDelta);

	// remove entity
	pDoc->DeleteEntity (EntityIndex);
	EntityIndex = -1;

	// select the model
	pDoc->ResetAllSelections ();

	// select the model
	OnSelectBrushes ();
}

void CModelDialog::OnLockorigin() 
{
	int Locked;
	Model *pModel;

	Locked = m_cbLockOrigin.GetCheck ();
	pModel = GetCurrentModel ();
	if (pModel != NULL)
	{
		Model_SetRotationLock (pModel, (Locked == 1));
	}
}

BOOL CModelDialog::OnInitDialog() 
{
	RECT	rParent ;
	RECT	rTabControl ;
	
	CDialog::OnInitDialog();
	
	m_pParentCtrl->GetClientRect( &rParent) ;
	m_pParentCtrl->GetItemRect( 0, &rTabControl ) ;

	rParent.top = rTabControl.bottom + FTC_BORDER_SIZE_TOP ;
	rParent.left = rParent.left + FTC_BORDER_SIZE_LEFT ;
	rParent.right = rParent.right - FTC_BORDER_SIZE_RIGHT ;
	rParent.bottom = rParent.bottom - FTC_BORDER_SIZE_BOTTOM ;
	
	SetWindowPos
	( 
		NULL,
		rParent.left,
		rParent.top,
		rParent.right,
		rParent.bottom,
		SWP_NOZORDER
	) ;

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CModelDialog::OnAddevent() 
{
    EventEditType EventInfo;
	CEventKeyEdit *pDlg;
	Model *pModel;

	pModel = GetCurrentModel ();
	if (pModel == NULL)
	{
		return;
	}
    EventInfo.AddOrEdit = 'A';	
	pDlg = new CEventKeyEdit (this, &EventInfo);
	if (pDlg != NULL)
	{
		if (pDlg->DoModal () == IDOK)
		{
			Model_AddEvent (pModel, EventInfo.Time, EventInfo.EventString);			
			UpdateKeysList (pModel);
		}
	}
}

void CModelDialog::OnDeleteevent() 
{
	Model *pModel;

	pModel = GetCurrentModel ();
	if (pModel != NULL)
	{
		int CurSel;
		geFloat Time, KeyTime;
		int KeyType;

    	CurSel = GetCurrentLbKey (&Time, &KeyType);

		if ((CurSel != LB_ERR) && (KeyType == KEY_TYPE_EVENT))
		{
			geXForm3d XfmFrom, XfmTo;
			
			geXForm3d_SetIdentity (&XfmFrom);
			geXForm3d_SetIdentity (&XfmTo);

			Model_DeleteEvent (pModel, Time);

			// get current transform
			KeyTime = Model_GetCurrentKeyTime (pModel);
			Model_GetKeyframe (pModel, KeyTime, &XfmFrom);

			ReverseDeltas (pModel);

			// position to key 0
			Model_GetKeyframe (pModel, 0.0f, &XfmTo);
			Model_TransformFromTo (pModel, &XfmFrom, &XfmTo, Level_GetBrushes (pDoc->pLevel));

			Model_SetCurrentKeyTime (pModel, 0.0f);

			// and finally update the views
			pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);

			if (Model_GetNumKeys (pModel) == 1)
			{
				Model_SetRotationLock (pModel, FALSE);
				m_cbLockOrigin.SetCheck (0);
				m_cbLockOrigin.EnableWindow (FALSE);
			}
			UpdateKeysList (pModel);
		}
	}
	
}

void CModelDialog::OnEditevent() 
{
	// TODO: Add your control notification handler code here
	
}

void CModelDialog::OnEditkey() 
{
    int CurSel;
    int KeyType;
    geFloat OldTime, NewTime;

    CurSel = GetCurrentLbKey (&OldTime, &KeyType);
	
	if ((CurSel <= 0) || (KeyType != KEY_TYPE_KEYFRAME))
	{
		return;
	}

	static const CString Prompt = "Enter new key time";
	CString Value;

	Value.Format ("%f", OldTime);

	Model *pModel = GetCurrentModel ();
	geBoolean GotKeyTime = GE_FALSE;
	CFloatKeyEditDlg Dlg (this, Prompt, &Value);
	do
	{
		int rslt = Dlg.DoModal ();

		if (rslt == IDOK)
		{
			geXForm3d XfmTemp;

			// get the new time.
			sscanf (Value, "%f", &NewTime);

			if (NewTime <= 0.0f)
			{
				MessageBox ("Can't replace the first key.", "Edit key", MB_ICONEXCLAMATION | MB_OK);
				GotKeyTime = GE_FALSE;
			}
			else if (Model_GetKeyframe (pModel, NewTime, &XfmTemp))
			// if a key already exists at this time, then we need to prompt user.
			{
				CString msg;

				msg.Format ("A key with time %s already exists.\rDo you want to replace it?", Value);
				
				rslt = MessageBox (msg, "Edit key", MB_ICONQUESTION | MB_YESNOCANCEL);
				switch (rslt)
				{
					case IDCANCEL :
					{
						ReverseDeltas (pModel);
						pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);
						return;
					}
					case IDNO :
						break;
					case IDYES :
						GotKeyTime = GE_TRUE;
						Model_DeleteKeyframe (pModel, NewTime);
						break;
				}
			}
			else
			{
				GotKeyTime = GE_TRUE;
			}
		}
		else
		{
			return;
		}
	} while (!GotKeyTime);

	// get key information
	geXForm3d XfmFrom;
	Model_GetKeyframe (pModel, OldTime, &XfmFrom);

	// delete the old key
	OnDeletekey ();

	// and then add this new key.
	AddTheKey (pModel, NewTime, &XfmFrom);
}

void CModelDialog::OnEditmodel() 
{
	// TODO: Add your control notification handler code here
	
}
