#include <ole2.h> 
#include <xmllite.h> 
#include <stdio.h> 
#include <shlwapi.h> 
#include "xml.h"
#include "xmlmap.h"
#include "backend.h"
#include "p11.h"

const UINT MAX_ELEMENT_DEPTH = 8;

#define SAFE_RELEASE(I)         if (I){ I->Release();  I = NULL; }
#define SAFE_FREE(val)			if (val != NULL){ free(val); val = NULL;}
#define FAILED_OUT(retVal)		if ((retVal) != 0){goto out;}

#define check_xml(call) if((rc = call) < 0) { \
	be_log(EID_VWR_LOG_DETAIL, TEXT("Error while dealing with file (calling '%s'): %d"), #call, rc); \
	goto out; \
}

/* Write elements to the description in *element */
static int write_elements(IXmlWriter *pWriter, struct element_desc *element) {
	HRESULT retVal;
	EID_CHAR* val = NULL;

	while (element->name) {
		if (element->label == NULL) {
			//assert(element->child_elements != NULL);
			//check_xml(xmlTextWriterStartElement(writer, BAD_CAST element->name));
			pWriter->WriteStartElement(NULL,element->name,NULL);
			if (element->attributes != NULL) {
				///write_attributes(pWriter, element->attributes);
				//retVal = pWriter->WriteAttributeString(NULL,
				//	element->name, NULL,
				//	element->attributes);
			}
			FAILED_OUT(retVal = write_elements(pWriter, element->child_elements));
			//check_xml(xmlTextWriterEndElement(writer));
			pWriter->WriteEndElement();
		}
		else {
			int have_cache = cache_have_label(element->label);
			//assert(element->child_elements == NULL);

			if (element->reqd && !have_cache) {
				be_log(EID_VWR_LOG_ERROR, TEXT("Could not write file: no data found for required label %s"), element->label);
				return -1;
			}
			if (have_cache) {
				val = cache_get_xmlform(element->label);
				if (!element->is_b64) {
					pWriter->WriteElementString(NULL, element->name, NULL, element->label);
					//check_xml(xmlTextWriterWriteElement(writer, BAD_CAST element->name, BAD_CAST cache_get_xmlform(element->label)));
				}
				else {
					const struct eid_vwr_cache_item *item = cache_get_data(element->label);
					//check_xml(xmlTextWriterStartElement(writer, BAD_CAST element->name));
					//check_xml(xmlTextWriterWriteBase64(writer, item->data, 0, item->len));
					//check_xml(xmlTextWriterEndElement(writer));
				}
				free(val);
				val = NULL;
			}
		}
		element++;
	}
	retVal = 0;
out:
	if (val != NULL) {
		free(val);
	}
	return retVal;
}

/* Called when we enter the FILE or TOKEN states.
Note: in theory it would be possible to just store the xml data we
read from a file in the deserialize event into the cache as-is.
However, that has a few downsides:
- If the file has invalid XML or superfluous data, we will write that
same data back later on.
- If we would want to modify the XML format at some undefined point
in the future, it is a good idea generally to ensure that we
already generate new XML data */
int eid_vwr_gen_xml(void* data) {

	HRESULT retVal = S_OK;
	IXmlWriter *pWriter = NULL;
	IXmlWriterOutput *pWriterOutput = NULL;
	IStream *pMemoryStream = NULL;
	BYTE* pwszContent = NULL;
	STATSTG ssStreamData = { 0 };

	// Opens writeable output stream.
	pMemoryStream = SHCreateMemStream(NULL, 0);
	if (pMemoryStream == NULL)
		return E_OUTOFMEMORY;

	// Creates the xml writer and generates the content.
	FAILED_OUT(retVal = CreateXmlWriter(__uuidof(IXmlWriter), (void**)&pWriter, NULL));
	retVal = CreateXmlWriterOutputWithEncodingName(pMemoryStream, NULL, L"utf-8", &pWriterOutput);
	retVal = pWriter->SetOutput(pWriterOutput);
	retVal = pWriter->SetProperty(XmlWriterProperty_Indent, TRUE);

	retVal = pWriter->WriteStartDocument(XmlStandalone_Omit);
	retVal = pWriter->WriteStartElement(NULL, L"eid", NULL);

//	retVal = write_elements(writer, toplevel);
//	retVal = pWriter->WriteWhitespace(L"\n");
//	retVal = pWriter->WriteCData(L"This is CDATA text.");
//	retVal = pWriter->WriteWhitespace(L"\n");
	retVal = pWriter->WriteEndDocument();
	retVal = pWriter->Flush();

	// Allocates enough memory for the xml content.
	
	retVal = pMemoryStream->Stat(&ssStreamData, STATFLAG_NONAME);
	SIZE_T cbSize = ssStreamData.cbSize.LowPart;

	pwszContent = new BYTE[cbSize];
	if (pwszContent == NULL)
	{
		be_log(EID_VWR_LOG_COARSE, TEXT("Could not generate XML format: error creating the xml buffer"));
		return E_OUTOFMEMORY;
	}
	// Copies the content from the stream to the buffer.
	LARGE_INTEGER position;
	position.QuadPart = 0;
	retVal = pMemoryStream->Seek(position, STREAM_SEEK_SET, NULL);
	SIZE_T cbRead;
	retVal = pMemoryStream->Read(pwszContent, cbSize, &cbRead);

	cache_add(TEXT("xml"), pwszContent, cbSize / sizeof(WCHAR));

	out:
	if (pwszContent != NULL) {
		free(pwszContent);
	}

	return retVal;
}


HRESULT ReadNodePending(IXmlReader *pReader, XmlNodeType *pNodeType, int retries)
{
	HRESULT retval = pReader->Read(pNodeType);
	int count = 0;
	while ((retval == E_PENDING) && (count < retries)) {
		Sleep(1000);
		retval = pReader->Read(pNodeType);
		count++;
	}
	return retval;
}


HRESULT StoreLocalName(WCHAR** nodeNames, const WCHAR* pwszLocalName)
{
	WCHAR* temp = (WCHAR*)realloc((*nodeNames), sizeof(WCHAR) * (wcslen(pwszLocalName)+1) );
	if (temp == NULL)
		return S_FALSE;

	wcscpy(temp, pwszLocalName);
	*nodeNames = temp;

	return S_OK;
}

int ConvertWCharToMultiByte(const wchar_t* wsIn, char** bsOut)
{
	size_t buffersize = 256;
	char* buffer = NULL;
	size_t numOfCharConverted = 0;

	//retrieve the needed buffersize
	wcstombs_s(&numOfCharConverted, NULL, 0, wsIn, 0);
	buffer = (char*)calloc(numOfCharConverted, 1);
	if (buffer == NULL)
		return -1;
	*bsOut = buffer;

	buffersize = numOfCharConverted;

	return wcstombs_s(&numOfCharConverted, buffer, buffersize, wsIn, buffersize - 1);
	
}

HRESULT StoreTextElement(const WCHAR* pwszValue, WCHAR* wcsNodeName)
{
//	EID_CHAR* nodeName = NULL;
//	EID_CHAR* value = NULL;
	void* val = NULL;

//	if ((ConvertWCharToMultiByte(wcsNodeName, &nodeName) == 0) &&
//		(ConvertWCharToMultiByte(pwszValue, &value) == 0))
//	{
		struct element_desc *desc = get_elemdesc((EID_CHAR*)wcsNodeName);
		/* If we recognize this element, parse it */
		if (desc != NULL) {
			int len = 0; {
				val = convert_from_xml(desc->label, pwszValue, &len);
				cache_add(desc->label, val, len);
				eid_vwr_p11_to_ui((const EID_CHAR*)(desc->label), (const void*)val, len);
				be_log(EID_VWR_LOG_DETAIL, TEXT("found data for label %s"), desc->label);
				val = NULL;
			}
		}
//	}
//	SAFE_FREE(nodeName);
//	SAFE_FREE(value);

	return S_OK;
}

HRESULT ParseAttributes(IXmlReader* pReader, const WCHAR* pwszLocalName)
{
	const WCHAR* pwszValue;
//	EID_CHAR* nodeName = NULL;
//	EID_CHAR* value = NULL;
	EID_CHAR* val = NULL;
	HRESULT retVal = pReader->MoveToFirstAttribute();

	if (retVal != S_OK)
	{
		return retVal;
	}
	else
	{
		while (pReader->MoveToNextAttribute() == S_OK)
		{
			if (!pReader->IsDefault())
			{
				FAILED_OUT(retVal = pReader->GetLocalName(&pwszLocalName, NULL));
				FAILED_OUT(retVal = pReader->GetValue(&pwszValue, NULL));

//				if ((ConvertWCharToMultiByte(pwszLocalName, &nodeName) == 0) &&
//					(ConvertWCharToMultiByte(pwszValue, &value) == 0))
//				{

					struct attribute_desc *desc = get_attdesc((const EID_CHAR*)pwszLocalName);
					/* If we recognize this element, parse it */
					if (desc != NULL) {
						int len = 0; {
							val = (EID_CHAR*)convert_from_xml(desc->label, pwszValue, &len);
							cache_add(desc->label, val, len);
							eid_vwr_p11_to_ui((const EID_CHAR*)(desc->label), (const void*)val, len);
							be_log(EID_VWR_LOG_DETAIL, TEXT("found data for label %s"), desc->label);
							val = NULL;
						}
					}
				}
//			}
		}
	}
out:
//	SAFE_FREE(nodeName);
//	SAFE_FREE(value);

	return retVal;
}

int eid_vwr_deserialize (const EID_CHAR* filename)
{
	HRESULT retVal = S_OK;
	IStream *pFileStream = NULL;
	IXmlReader *pReader = NULL;
	XmlNodeType nodeType;
	const WCHAR* pwszLocalName;
	EID_CHAR* nodeName = NULL;
	EID_CHAR* value = NULL;
	EID_CHAR* val = NULL;
	const WCHAR* pwszValue;
	UINT attrCount = 0;
	UINT count = 0;
	UINT depth = 0;
	WCHAR* nodeNames[MAX_ELEMENT_DEPTH+1]; //also storing depth0

	for (count = 0; count <= MAX_ELEMENT_DEPTH; count++)
	{
		nodeNames[count] = NULL;
	}

	FAILED_OUT(retVal = SHCreateStreamOnFile(filename, STGM_READ, &pFileStream));
	FAILED_OUT(retVal = CreateXmlReader(__uuidof(IXmlReader), (void**)&pReader, NULL));
	FAILED_OUT(retVal = pReader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit));
	FAILED_OUT(retVal = pReader->SetProperty(XmlReaderProperty_MaxElementDepth, MAX_ELEMENT_DEPTH));
	FAILED_OUT(retVal = pReader->SetInput(pFileStream));

	retVal = ReadNodePending(pReader, &nodeType, 20);

	while (retVal == S_OK )
	{
		switch (nodeType)
		{
		case XmlNodeType_XmlDeclaration:
			break;
		case XmlNodeType_Element:
			FAILED_OUT(retVal = pReader->GetLocalName(&pwszLocalName, NULL));
			FAILED_OUT(retVal = pReader->GetAttributeCount(&attrCount));
			FAILED_OUT(retVal = pReader->GetDepth(&depth));
			if(depth <= MAX_ELEMENT_DEPTH)
				FAILED_OUT(retVal = StoreLocalName(&(nodeNames[depth]), pwszLocalName ));
			if (attrCount > 0)
			{
				retVal = ParseAttributes(pReader, pwszLocalName);
				//continue with other nodes, even if this one failed, so don't check return value
			}
			break;
		case XmlNodeType_EndElement:
			break;
		case XmlNodeType_Text:
			FAILED_OUT(retVal = pReader->GetValue(&pwszValue, NULL));
			FAILED_OUT(retVal = pReader->GetDepth(&depth));
			if (depth <= MAX_ELEMENT_DEPTH)
				FAILED_OUT(retVal = StoreTextElement(pwszValue, nodeNames[depth-1]));
			break;
		case XmlNodeType_Whitespace:
		case XmlNodeType_CDATA:
		case XmlNodeType_ProcessingInstruction:
		case XmlNodeType_Comment:
		case XmlNodeType_DocumentType:
			break;
		}
		retVal = ReadNodePending(pReader, &nodeType, 20);
	}

out:
	SAFE_RELEASE(pFileStream);
	SAFE_RELEASE(pReader);

	for (count = 0; count <= MAX_ELEMENT_DEPTH; count++)
	{
		if (nodeNames[count] != NULL)
		{
			free(nodeNames[count]);
			nodeNames[count] = NULL;
		}
	}
	return 0;
}

int eid_vwr_serialize(const EID_CHAR* filename) {
	const struct eid_vwr_cache_item* item = cache_get_data(TEXT("xml"));
	FILE* f = EID_FOPEN(filename, TEXT("w"));
	fwrite(item->data, item->len, 1, f);
	return fclose(f);
}
