#ifndef _NAMED_TYPE_OBJ
#define _NAMED_TYPE_OBJ

#include <string>
#include "../STJUtils/STJConsts.h"
#include "../STJUtils/STJPara.h"

class CNamedTypeObj : public CObject
{
	DECLARE_DYNCREATE(CNamedTypeObj);

protected:
	static std::string CNamedTypeObj_baseType;
	static std::string CNamedTypeObj_subType;

public:
	CNamedTypeObj();
	CNamedTypeObj(const CNamedTypeObj &rhs);
	CNamedTypeObj &operator=(const CNamedTypeObj &rhs);
	virtual ~CNamedTypeObj() {}
public:
	virtual const std::string &getBaseType() const { return CNamedTypeObj_baseType; }
	virtual const std::string &getSubType() const{return CNamedTypeObj_subType; }

	//name
	virtual void setName(const std::string &name) {m_objName = name; }
	virtual const std::string &getName() const{ return m_objName; }

	virtual void popupMenu(void *data);
	virtual HTREEITEM displayInTree(CTreeCtrl &tree, HTREEITEM parent) const;
	virtual HTREEITEM displaySelfInTree(CTreeCtrl &tree, HTREEITEM parent) const;
	virtual void render(const RENDER_PARA &para) const;
	virtual void render_select(const RENDER_PARA &para) const;
public:
	int isVisible() const{ return m_flag & VISIBLE_BIT; };
	void setVisible(bool val)
	{
		if (val)
			m_flag |= VISIBLE_BIT;
		else
			m_flag = (m_flag & ~VISIBLE_BIT);
	}
	int isSelect() const{ return m_flag & SELECT_BIT; };
	void setSelect(bool val)
	{
		if (val)
			m_flag |= SELECT_BIT;
		else
			m_flag = (m_flag & ~SELECT_BIT);
	}
	void setDisplayInTree(bool flag);
	bool isDisplayInTree() const;
protected:
	std::string  m_objName;
	int m_flag;
	bool m_bDisplayInTree;
};

#endif