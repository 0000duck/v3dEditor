#pragma once

#include <atlbase.h>
#include <xmllite.h>

namespace ve {

	namespace xml {

		struct Attribute
		{
			StringW name;
			StringW value;
		};

		struct Element
		{
		public:
			StringW name;
			collection::Vector<Attribute> attributes;

			StringW value;

			Element* pParent;
			collection::Vector<Element*> childs;

			static Element* Create()
			{
				return VE_NEW_T(Element);
			}

			void Destroy()
			{
				auto it_child_begin = childs.begin();
				auto it_child_end = childs.end();

				for (auto it_child = childs.begin(); it_child != it_child_end; ++it_child)
				{
					(*it_child)->Destroy();
				}

				VE_DELETE_THIS_T(this, Element);
			}

		private:
			Element() :
				pParent(nullptr)
			{
			}

			VE_DECLARE_ALLOCATOR
		};

		static Element* AddChild(Element* pParent, const wchar_t* pName)
		{
			Element* pChild = Element::Create();
			if (pChild == nullptr)
			{
				return nullptr;
			}

			pChild->pParent = pParent;
			pChild->name = pName;

			pParent->childs.push_back(pChild);

			return pChild;
		}

		static const Attribute* FindAttribute(const Element* pElement, const wchar_t* pName)
		{
			auto it = std::find_if(pElement->attributes.begin(), pElement->attributes.end(), [pName](const Attribute& attr) { return attr.name == pName; });
			if (it == pElement->attributes.end())
			{
				return nullptr;
			}

			return &(*it);
		}

		static const bool AddAttribute(Element* pElement, const wchar_t* pName, const wchar_t* pValue)
		{
			auto it = std::find_if(pElement->attributes.begin(), pElement->attributes.end(), [pName](const Attribute& attr) { return attr.name == pName; });
			if (it != pElement->attributes.end())
			{
				return false;
			}

			Attribute attr;
			attr.name = pName;
			attr.value = pValue;
			pElement->attributes.push_back(attr);

			return true;
		}

		static Element* Load(const wchar_t* pFilePath)
		{
			CComPtr<IXmlReader> reader;

			if (FAILED(CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void**>(&reader), 0)))
			{
				return false;
			}

			CComPtr<IStream> pStream;
			if (FAILED(SHCreateStreamOnFile(pFilePath, STGM_READ, &pStream)))
			{
				return false;
			}

			if (FAILED(reader->SetInput(pStream)))
			{
				return false;
			}

			Element* pDocument = Element::Create();
			if (pDocument == nullptr)
			{
				return nullptr;
			}

			XmlNodeType nodeType;
			collection::Vector<Element*> elementStack;

			while(reader->Read(&nodeType) == S_OK)
			{
				if (nodeType == XmlNodeType_Element)
				{
					Element* pElement = Element::Create();
					if (pElement == nullptr)
					{
						pDocument->Destroy();
						return nullptr;
					}

					if (elementStack.empty() == false)
					{
						Element* pParent = elementStack.back();
						pElement->pParent = pParent;
						pParent->childs.push_back(pElement);
					}
					else
					{
						pDocument->childs.push_back(pElement);
					}

					LPCWSTR pName;
					if (SUCCEEDED(reader->GetLocalName(&pName, nullptr)))
					{
						pElement->name = pName;
					}
					else
					{
						pDocument->Destroy();
						return nullptr;
					}

					if (SUCCEEDED(reader->MoveToFirstAttribute()))
					{
						do
						{
							LPCWSTR pAttributeName;
							if (SUCCEEDED(reader->GetLocalName(&pAttributeName, nullptr)))
							{
								LPCWSTR pAttributeValue;
								if (SUCCEEDED(reader->GetValue(&pAttributeValue, nullptr)))
								{
									Attribute attr;
									attr.name = pAttributeName;
									attr.value = pAttributeValue;
									pElement->attributes.push_back(attr);
								}
								else
								{
									pDocument->Destroy();
									return nullptr;
								}
							}
							else
							{
								pDocument->Destroy();
								return nullptr;
							}

						} while (reader->MoveToNextAttribute() == S_OK);
					}

					elementStack.push_back(pElement);
				}
				else if (nodeType == XmlNodeType_Text)
				{
					LPCWSTR pValue;
					if (SUCCEEDED(reader->GetValue(&pValue, nullptr)))
					{
						elementStack.back()->value = pValue;
					}
					else
					{
						pDocument->Destroy();
						return nullptr;
					}
				}
				else if (nodeType == XmlNodeType_EndElement)
				{
					elementStack.pop_back();
				}
			}

			return pDocument;
		}

		static bool Save(CComPtr<IXmlWriter>& writer, Element* pElement)
		{
			if (FAILED(writer->WriteStartElement(nullptr, pElement->name.c_str(), nullptr)))
			{
				return false;
			}

			auto it_attr_begin = pElement->attributes.begin();
			auto it_attr_end = pElement->attributes.end();

			for (auto it_attr = it_attr_begin; it_attr != it_attr_end; ++it_attr)
			{
				if (FAILED(writer->WriteAttributeString(nullptr, it_attr->name.c_str(), nullptr, it_attr->value.c_str())))
				{
					return false;
				}
			}

			if (pElement->value.empty() == false)
			{
				if (FAILED(writer->WriteString(pElement->value.c_str())))
				{
					return false;
				}
			}

			auto it_begin = pElement->childs.begin();
			auto it_end = pElement->childs.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				if (Save(writer, *it) == false)
				{
					return false;
				}
			}

			if (FAILED(writer->WriteFullEndElement()))
			{
				return false;
			}

			return true;
		}

		static bool Save(const wchar_t* pFilePath, Element* pDocument)
		{
			CComPtr<IXmlWriter> writer;
			if (FAILED(CreateXmlWriter(__uuidof(IXmlWriter), reinterpret_cast<void**>(&writer), 0)))
			{
				return false;
			}
			
			CComPtr<IStream> stream;
			if (FAILED(SHCreateStreamOnFile(pFilePath, STGM_CREATE | STGM_WRITE, &stream)))
			{
				return false;
			}
			
			if (FAILED(writer->SetOutput(stream)))
			{
				return false;
			}
			
			if (FAILED(writer->SetProperty(XmlWriterProperty_Indent, TRUE)))
			{
				return false;
			}

			if (FAILED(writer->WriteStartDocument(XmlStandalone_Omit)))
			{
				return false;
			}

			auto it_begin = pDocument->childs.begin();
			auto it_end = pDocument->childs.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				if (Save(writer, *it) == false)
				{
					return false;
				}
			}

			if (FAILED(writer->WriteEndDocument()))
			{
				return false;
			}

			if (FAILED(writer->Flush()))
			{
				return false;
			}

			return true;
		}

	}
}