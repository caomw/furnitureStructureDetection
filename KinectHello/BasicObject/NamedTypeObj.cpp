#include "stdafx.h"
#include "NamedTypeObj.h"

IMPLEMENT_DYNCREATE(CNamedTypeObj, CObject)

std::string CNamedTypeObj::CNamedTypeObj_baseType = "Base";
std::string CNamedTypeObj::CNamedTypeObj_subType = "NULL";

CNamedTypeObj::CNamedTypeObj()
{
	m_objName = UNKNOWN_NAME;
	m_flag = 0x5;
	m_bDisplayInTree = true;
}

HTREEITEM CNamedTypeObj::displayInTree(CTreeCtrl &tree, HTREEITEM parent) const
{
	if(!m_bDisplayInTree)
		return NULL;
	HTREEITEM hItem = tree.InsertItem(m_objName.c_str(),parent);
	tree.SetItemData(hItem, (DWORD_PTR)this);
	return hItem;
}

HTREEITEM CNamedTypeObj::displaySelfInTree(CTreeCtrl &tree, HTREEITEM parent) const
{
	return CNamedTypeObj::displayInTree(tree,parent);
}


void CNamedTypeObj::popupMenu(void *data)
{

}

CNamedTypeObj::CNamedTypeObj(const CNamedTypeObj &rhs)
{
	m_objName = rhs.m_objName;
	m_flag = rhs.m_flag;
	m_bDisplayInTree = rhs.m_bDisplayInTree;
}

CNamedTypeObj &CNamedTypeObj::operator=(const CNamedTypeObj &rhs)
{
	m_objName = rhs.m_objName;
	m_flag = rhs.m_flag;
	m_bDisplayInTree = rhs.m_bDisplayInTree;
	return *this;
}

void CNamedTypeObj::setDisplayInTree(bool flag)
{
	m_bDisplayInTree = flag;
}

bool CNamedTypeObj::isDisplayInTree() const
{
	return m_bDisplayInTree;
}

void CNamedTypeObj::render(const RENDER_PARA &para) const
{

}

void CNamedTypeObj::render_select(const RENDER_PARA &para) const
{

}