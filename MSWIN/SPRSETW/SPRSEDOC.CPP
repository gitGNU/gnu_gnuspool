// sprsedoc.cpp : implementation of the CSprsetwDoc class
//

#include "stdafx.h"
#include "sprsetw.h"

#include "sprsedoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSprsetwDoc

IMPLEMENT_DYNCREATE(CSprsetwDoc, CDocument)

BEGIN_MESSAGE_MAP(CSprsetwDoc, CDocument)
	//{{AFX_MSG_MAP(CSprsetwDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSprsetwDoc construction/destruction

CSprsetwDoc::CSprsetwDoc()
{
	// TODO: add one-time construction code here
}

CSprsetwDoc::~CSprsetwDoc()
{
}

BOOL CSprsetwDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	SetTitle("Set Parameters");
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSprsetwDoc diagnostics

#ifdef _DEBUG
void CSprsetwDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSprsetwDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprsetwDoc commands
