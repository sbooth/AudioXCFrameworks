#include "All.h"
#define APE_ENABLE_TAG_FOOTER
#include "APETag.h"
#include "CharacterHelper.h"
#include "IO.h"
#include "GlobalFunctions.h"

namespace APE
{

/**************************************************************************************************
CAPETagField
**************************************************************************************************/
CAPETagField::CAPETagField(const str_utfn * pFieldName, const void * pFieldValue, int nFieldBytes, int nFlags)
{
    // field name
    size_t nFieldNameLength = wcslen(pFieldName);
    m_spFieldNameUTF16.Assign(new str_utfn [nFieldNameLength + 1], true);
    memcpy(m_spFieldNameUTF16, pFieldName, (nFieldNameLength + 1) * sizeof(m_spFieldNameUTF16[0]));

    // data (we'll always allocate two extra bytes and memset to 0 so we're safely NULL terminated)
    m_nFieldValueBytes = ape_max(nFieldBytes, 0);
    m_spFieldValue.Assign(new char [static_cast<size_t>(m_nFieldValueBytes) + 2], true);
    memset(m_spFieldValue, 0, static_cast<size_t>(m_nFieldValueBytes) + 2);
    if (m_nFieldValueBytes > 0)
        memcpy(m_spFieldValue, pFieldValue, static_cast<size_t>(m_nFieldValueBytes));

    // flags
    m_nFieldFlags = nFlags;
}

CAPETagField::~CAPETagField()
{
    m_spFieldNameUTF16.Delete();
    m_spFieldValue.Delete();
}

int CAPETagField::GetFieldSize() const
{
    CSmartPtr<char> spFieldNameANSI(CAPECharacterHelper::GetANSIFromUTF16(m_spFieldNameUTF16), true);
    return (static_cast<int>(strlen(spFieldNameANSI) + 1)) + m_nFieldValueBytes + 4 + 4;
}

const str_utfn * CAPETagField::GetFieldName() const
{
    return m_spFieldNameUTF16;
}

const char * CAPETagField::GetFieldValue()
{
    return m_spFieldValue;
}

int CAPETagField::GetFieldValueSize() const
{
    return m_nFieldValueBytes;
}

int CAPETagField::GetFieldFlags() const
{
    return m_nFieldFlags;
}

void CAPETagField::Save32(char * pBuffer, int nValue)
{
    pBuffer[0] = static_cast<char>(nValue);
    pBuffer[1] = static_cast<char>(nValue >> 8);
    pBuffer[2] = static_cast<char>(nValue >> 16);
    pBuffer[3] = static_cast<char>(nValue >> 24);
}

int CAPETagField::SaveField(char * pBuffer, int nBytes)
{
    Save32(pBuffer, m_nFieldValueBytes);
    pBuffer += 4;
    Save32(pBuffer, m_nFieldFlags);
    pBuffer += 4;
    nBytes -= 8;

    CSmartPtr<char> spFieldNameANSI(static_cast<char *>(CAPECharacterHelper::GetANSIFromUTF16(m_spFieldNameUTF16)), true);
    strcpy_s(pBuffer, static_cast<size_t>(nBytes), spFieldNameANSI);
    pBuffer += strlen(spFieldNameANSI) + 1;
    nBytes -= static_cast<int>(strlen(spFieldNameANSI)) + 1;

    memcpy(pBuffer, m_spFieldValue, static_cast<size_t>(ape_min(m_nFieldValueBytes, nBytes)));

    return GetFieldSize();
}

bool CAPETagField::GetIsReadOnly() const
{
    return (m_nFieldFlags & TAG_FIELD_FLAG_READ_ONLY) ? true : false;
}

bool CAPETagField::GetIsUTF8Text() const
{
    return ((m_nFieldFlags & TAG_FIELD_FLAG_DATA_TYPE_MASK) == TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8) ? true : false;
}

void CAPETagField::SetFieldFlags(int nFlags)
{
    m_nFieldFlags = nFlags;
}

/**************************************************************************************************
CAPETag
**************************************************************************************************/
const wchar_t * CAPETag::s_aryID3GenreNames[CAPETag::s_nID3GenreCount] =
{
    L"Blues", L"Classic Rock", L"Country", L"Dance", L"Disco", L"Funk", L"Grunge", L"Hip-Hop",
    L"Jazz", L"Metal", L"New Age", L"Oldies", L"Other", L"Pop", L"R&B", L"Rap", L"Reggae", L"Rock", L"Techno",
    L"Industrial", L"Alternative", L"Ska", L"Death Metal", L"Pranks", L"Soundtrack", L"Euro-Techno", L"Ambient",
    L"Trip-Hop", L"Vocal", L"Jazz+Funk", L"Fusion", L"Trance", L"Classical", L"Instrumental", L"Acid", L"House", L"Game",
    L"Sound Clip", L"Gospel", L"Noise", L"AlternRock", L"Bass", L"Soul", L"Punk", L"Space", L"Meditative", L"Instrumental Pop",
    L"Instrumental Rock", L"Ethnic", L"Gothic", L"Darkwave", L"Techno-Industrial", L"Electronic", L"Pop-Folk", L"Eurodance",
    L"Dream", L"Southern Rock", L"Comedy", L"Cult", L"Gangsta", L"Top 40", L"Christian Rap", L"Pop/Funk", L"Jungle",
    L"Native American", L"Cabaret", L"New Wave", L"Psychedelic", L"Rave", L"Showtunes", L"Trailer", L"Lo-Fi", L"Tribal",
    L"Acid Punk", L"Acid Jazz", L"Polka", L"Retro", L"Musical", L"Rock & Roll", L"Hard Rock", L"Folk", L"Folk-Rock", L"National Folk",
    L"Swing", L"Fast Fusion", L"Bebop", L"Latin", L"Revival", L"Celtic", L"Bluegrass", L"Avantgarde", L"Gothic Rock", L"Progressive Rock",
    L"Psychedelic Rock", L"Symphonic Rock", L"Slow Rock", L"Big Band", L"Chorus", L"Easy Listening", L"Acoustic", L"Humour",
    L"Speech", L"Chanson", L"Opera", L"Chamber Music", L"Sonata", L"Symphony", L"Booty Bass", L"Primus", L"Porn Groove",
    L"Satire", L"Slow Jam", L"Club", L"Tango", L"Samba", L"Folklore", L"Ballad", L"Power Ballad", L"Rhythmic Soul", L"Freestyle",
    L"Duet", L"Punk Rock", L"Drum Solo", L"Acapella", L"Euro-House", L"Dance Hall", L"Goa", L"Drum & Bass", L"Club House", L"Hardcore",
    L"Terror", L"Indie", L"BritPop", L"Black Punk", L"Polsk Punk", L"Beat", L"Christian Gangsta", L"Heavy Metal", L"Black Metal",
    L"Crossover", L"Contemporary C", L"Christian Rock", L"Merengue", L"Salsa", L"Thrash Metal", L"Anime", L"JPop", L"SynthPop"
};

CAPETag::CAPETag(const str_utfn * pFilename, bool bAnalyze)
{
    m_spIO.Assign(CreateCIO());
    m_spIO->Open(pFilename);

    m_bAnalyzed = false;
    m_nFields = 0;
    m_nAllocatedFields = 0;
    m_aryFields = APE_NULL;
    m_nTagBytes = 0;
    m_bIgnoreReadOnly = false;
    m_bHasAPETag = false;
    m_bHasID3Tag = false;
    m_nAPETagVersion = -1;
    m_bCheckForID3v1 = true;

    if (bAnalyze)
        Analyze();
}

CAPETag::CAPETag(CIO * pIO, bool bAnalyze, bool bCheckForID3v1)
{
    m_spIO.Assign(pIO, false, false); // we don't own the IO source
    m_bAnalyzed = false;
    m_nFields = 0;
    m_nAllocatedFields = 0;
    m_aryFields = APE_NULL;
    m_nTagBytes = 0;
    m_bIgnoreReadOnly = false;
    m_bHasAPETag = false;
    m_bHasID3Tag = false;
    m_nAPETagVersion = -1;
    m_bCheckForID3v1 = bCheckForID3v1;

    if (bAnalyze)
    {
        Analyze();
    }
}

CAPETag::~CAPETag()
{
    CAPETag::ClearFields(); // call CAPETag because calling the interface function is gone
    APE_SAFE_ARRAY_DELETE(m_aryFields)
}

bool CAPETag::GetAnalyzed()
{
    return m_bAnalyzed;
}

int CAPETag::GetTagBytes()
{
    if (!m_bAnalyzed) { Analyze(); }

    return m_nTagBytes;
}

CAPETagField * CAPETag::GetTagField(int nIndex)
{
    if (!m_bAnalyzed) { Analyze(); }

    if ((nIndex >= 0) && (nIndex < m_nFields))
    {
        return m_aryFields[nIndex];
    }

    return APE_NULL;
}

int CAPETag::Save(bool bUseOldID3)
{
    if (Remove(false) != ERROR_SUCCESS)
        return ERROR_UNDEFINED;

    if (m_nFields == 0) { return ERROR_SUCCESS; }

    int nResult = ERROR_UNDEFINED;

    if (bUseOldID3 == false)
    {
        int z = 0;

        // calculate the size of the whole tag
        int nFieldBytes = 0;
        for (z = 0; z < m_nFields; z++)
            nFieldBytes += m_aryFields[z]->GetFieldSize();

        // sort the fields
        SortFields();

        // build the footer
        APE_TAG_FOOTER APETagFooter(m_nFields, nFieldBytes);

        // make a buffer for the tag
        const int nTotalTagBytes = APETagFooter.GetTotalTagBytes();
        CSmartPtr<char> spRawTag(new char [static_cast<size_t>(nTotalTagBytes)], true);

        // save the fields
        int nLocation = 0;
        for (z = 0; z < m_nFields; z++)
            nLocation += m_aryFields[z]->SaveField(&spRawTag[nLocation], nTotalTagBytes - nLocation);

        // add the footer to the buffer
        memcpy(&spRawTag[nLocation], &APETagFooter, APE_TAG_FOOTER_BYTES);
        //nLocation += APE_TAG_FOOTER_BYTES; // we grow by this many, but nothing ever uses nLocation again

        // dump the tag to the I/O source
        nResult = WriteBufferToEndOfIO(spRawTag, nTotalTagBytes);
    }
    else if (bUseOldID3 && (sizeof(ID3_TAG) == ID3_TAG_BYTES))
    {
        // build the ID3 tag
        ID3_TAG ID3Tag;
        nResult = CreateID3Tag(&ID3Tag);
        if (nResult == ERROR_SUCCESS)
            nResult = WriteBufferToEndOfIO(&ID3Tag, ID3_TAG_BYTES);
    }

    return nResult;
}

int CAPETag::WriteBufferToEndOfIO(void * pBuffer, int nBytes)
{
    const int64 nOriginalPosition = m_spIO->GetPosition();

    unsigned int nBytesWritten = 0;
    m_spIO->Seek(0, SeekFileEnd);

    const int nResult = m_spIO->Write(pBuffer, static_cast<unsigned int>(nBytes), &nBytesWritten);

    m_spIO->Seek(nOriginalPosition, SeekFileBegin);

    return nResult;
}

int CAPETag::Analyze()
{
    // clean-up
    ID3_TAG ID3Tag; APE_CLEAR(ID3Tag);
    ClearFields();
    m_nTagBytes = 0;

    m_bAnalyzed = true;

    // store the original location
    const int64 nOriginalPosition = m_spIO->GetPosition();

    // remove tag markers
    m_bHasID3Tag = false;
    m_bHasAPETag = false;
    m_nAPETagVersion = -1;

    // check for an ID3v1 tag
    if (m_bCheckForID3v1 && (sizeof(ID3Tag) == ID3_TAG_BYTES) && (m_spIO->GetSize() > ID3_TAG_BYTES))
    {
        if (m_spIO->Seek(-ID3_TAG_BYTES, SeekFileEnd) == ERROR_SUCCESS)
        {
            unsigned int nBytesRead = 0;
            const int nReadRetVal = m_spIO->Read(&ID3Tag, ID3_TAG_BYTES, &nBytesRead);
            if ((nBytesRead == ID3_TAG_BYTES) && (nReadRetVal == 0))
            {
                if (ID3Tag.Header[0] == 'T' && ID3Tag.Header[1] == 'A' && ID3Tag.Header[2] == 'G')
                {
                    m_bHasID3Tag = true;
                    m_nTagBytes += ID3_TAG_BYTES;
                }
            }
        }
    }

    // set the fields
    if (m_bHasID3Tag)
    {
        SetFieldID3String(APE_TAG_FIELD_ARTIST, ID3Tag.Artist, 30);
        SetFieldID3String(APE_TAG_FIELD_ALBUM, ID3Tag.Album, 30);
        SetFieldID3String(APE_TAG_FIELD_TITLE, ID3Tag.Title, 30);
        SetFieldID3String(APE_TAG_FIELD_COMMENT, ID3Tag.Comment, 28);
        SetFieldID3String(APE_TAG_FIELD_YEAR, ID3Tag.Year, 4);

        char cTemp[16];
        APE_CLEAR(cTemp);
        #ifdef _MSC_VER
            _itoa_s(ID3Tag.Track, cTemp, 16, 10);
        #else
            snprintf(cTemp, 16, "%d", ID3Tag.Track);
        #endif
        SetFieldString(APE_TAG_FIELD_TRACK, cTemp, false);

        if ((ID3Tag.Genre == CAPETag::s_nID3GenreUndefined) || (ID3Tag.Genre >= CAPETag::s_nID3GenreCount))
            SetFieldString(APE_TAG_FIELD_GENRE, APE_TAG_GENRE_UNDEFINED);
        else
            SetFieldString(APE_TAG_FIELD_GENRE, CAPETag::s_aryID3GenreNames[ID3Tag.Genre]);
    }

    // try loading the APE tag
    // we used to not load if we found an ID3 tag, but somebody sent me a file with both an ID3 tag and APE tag, so we'll try to parse that going forward
    {
        APE_TAG_FOOTER APETagFooter;
        int nTagBytes = static_cast<int>(APE_TAG_FOOTER_BYTES + (m_bHasID3Tag ? ID3_TAG_BYTES : 0));
        if (m_spIO->Seek(-nTagBytes, SeekFileEnd) == ERROR_SUCCESS)
        {
            unsigned int nBytesRead = 0;
            APETagFooter.Empty();
            int nReadRetVal = m_spIO->Read(&APETagFooter, APE_TAG_FOOTER_BYTES, &nBytesRead);
            if ((nBytesRead == APE_TAG_FOOTER_BYTES) && (nReadRetVal == 0))
            {
                if (APETagFooter.GetIsValid(false))
                {
                    m_bHasAPETag = true;
                    m_nAPETagVersion = APETagFooter.GetVersion();

                    const int nRawFieldBytes = APETagFooter.GetFieldBytes();
                    m_nTagBytes += APETagFooter.GetTotalTagBytes();

                    CSmartPtr<char> spRawTag(new char [static_cast<size_t>(nRawFieldBytes)], true);
                    if (m_spIO->Seek(static_cast<int64>(-(APETagFooter.GetTotalTagBytes() - APETagFooter.GetFieldsOffset() + (m_bHasID3Tag ? ID3_TAG_BYTES : 0))), SeekFileEnd) == ERROR_SUCCESS)
                    {
                        nReadRetVal = m_spIO->Read(spRawTag.GetPtr(), static_cast<unsigned int>(nRawFieldBytes), &nBytesRead);

                        if ((nReadRetVal == 0) && (nRawFieldBytes == static_cast<int>(nBytesRead)))
                        {
                            // parse out the raw fields
                            int nLocation = 0;
                            for (int z = 0; z < APETagFooter.GetNumberFields(); z++)
                            {
                                const int nMaximumFieldBytes = nRawFieldBytes - nLocation;

                                int nBytes = 0;
                                if (LoadField(&spRawTag[nLocation], nMaximumFieldBytes, &nBytes) != ERROR_SUCCESS)
                                {
                                    // if LoadField(...) fails, it means that the tag is corrupt (accidentally or intentionally)
                                    // we'll just bail out -- leaving the fields we've already set
                                    break;
                                }
                                nLocation += nBytes;
                            }
                        }
                    }
                }
            }
        }
    }

    // restore the file pointer
    m_spIO->Seek(nOriginalPosition, SeekFileBegin);

    return ERROR_SUCCESS;
}

int CAPETag::ClearFields()
{
    for (int z = 0; z < m_nFields; z++)
    {
        APE_SAFE_DELETE(m_aryFields[z])
    }

    m_nFields = 0;

    return ERROR_SUCCESS;
}

int CAPETag::GetTagFieldIndex(const str_utfn * pFieldName)
{
    if (!m_bAnalyzed) { Analyze(); }
    if (pFieldName != APE_NULL)
    {
        for (int z = 0; z < m_nFields; z++)
        {
            if (StringIsEqual(m_aryFields[z]->GetFieldName(), pFieldName, false))
                return z;
        }
    }
    return -1;
}

CAPETagField * CAPETag::GetTagField(const str_utfn * pFieldName)
{
    int nIndex = GetTagFieldIndex(pFieldName);
    return (nIndex != -1) ? m_aryFields[nIndex] : APE_NULL;
}

int CAPETag::GetFieldString(const str_utfn * pFieldName, str_ansi * pBuffer, int * pBufferCharacters, bool bUTF8Encode)
{
    const int nOriginalCharacters = *pBufferCharacters;
    CSmartPtr<str_utfn> spUTF16(new str_utfn[static_cast<size_t>(*pBufferCharacters) + 1], true);
    spUTF16[0] = 0;

    int nResult = GetFieldString(pFieldName, spUTF16, pBufferCharacters);
    if (nResult == ERROR_SUCCESS)
    {
        CSmartPtr<str_ansi> spANSI(bUTF8Encode ? reinterpret_cast<str_ansi *>(CAPECharacterHelper::GetUTF8FromUTF16(spUTF16)) : CAPECharacterHelper::GetANSIFromUTF16(spUTF16), true);
        if (static_cast<int>(strlen(spANSI)) > nOriginalCharacters)
        {
            memset(pBuffer, 0, static_cast<size_t>(nOriginalCharacters) * sizeof(pBuffer[0]));
            *pBufferCharacters = 0;
            nResult = ERROR_UNDEFINED;
        }
        else
        {
            strcpy_s(pBuffer, static_cast<size_t>(nOriginalCharacters), spANSI);
            *pBufferCharacters = static_cast<int>(strlen(spANSI));
        }
    }

    spUTF16.Delete();

    return nResult;
}

int CAPETag::GetFieldString(const str_utfn * pFieldName, str_utfn * pBuffer, int * pBufferCharacters, const str_utfn * pListDelimiter)
{
    if (!m_bAnalyzed) { Analyze(); }

    int nResult = ERROR_UNDEFINED;

    if ((pBuffer != APE_NULL) && (*pBufferCharacters > 0) && (pListDelimiter != APE_NULL))
    {
        // empty result
        pBuffer[0] = 0;

        // get field
        CAPETagField * pAPETagField = GetTagField(pFieldName);
        if (pAPETagField == APE_NULL)
        {
            // the field doesn't exist -- return an empty string
            memset(pBuffer, 0, static_cast<size_t>(*pBufferCharacters) * sizeof(pBuffer[0]));
            *pBufferCharacters = 0;
        }
        else if (pAPETagField->GetIsUTF8Text() || (m_nAPETagVersion < 2000))
        {
            // find next NULL
            int nListItemStartIndex = 0;
            int nOutputCharacters = 0;
            const int nListDelimiterCharacters = static_cast<int>(wcslen(pListDelimiter));

            // loop each list item
            nResult = ERROR_SUCCESS;
            while (nListItemStartIndex < pAPETagField->GetFieldValueSize())
            {
                // get the value in UTF-16 format
                CSmartPtr<str_utfn> spUTF16;
                if (m_nAPETagVersion >= 2000)
                    spUTF16.Assign(CAPECharacterHelper::GetUTF16FromUTF8(reinterpret_cast<const str_utf8 *>(&pAPETagField->GetFieldValue()[nListItemStartIndex])), true);
                else
                    spUTF16.Assign(CAPECharacterHelper::GetUTF16FromANSI(&pAPETagField->GetFieldValue()[nListItemStartIndex]), true);

                // get the number of characters
                const int nCharacters = (static_cast<int>(wcslen(spUTF16)) + 1);

                int nNeededCharacters = nOutputCharacters + nCharacters;
                if (pBuffer[0] != 0)
                    nNeededCharacters += nListDelimiterCharacters;

                if (nNeededCharacters > *pBufferCharacters)
                {
                    // we'll fail here, because it's not clear what would get returned (null termination, size, etc.)
                    // and we really don't want to cause buffer overruns on the client side
                    // we use (nListDelimiterCharacters - 1) since there's already a NULL delimiter in the field
                    *pBufferCharacters = (pAPETagField->GetFieldValueSize() + 1) + ((nListDelimiterCharacters - 1) * 64); // worst-case scenario
                    nResult = ERROR_BAD_PARAMETER;
                    break;
                }
                else
                {
                    // copy list item to output

                    // delimiter
                    if (pBuffer[0] != 0)
                    {
                        wcscat_s(pBuffer, static_cast<size_t>(*pBufferCharacters), pListDelimiter);
                        nOutputCharacters += nListDelimiterCharacters;
                    }

                    // value
                    wcscat_s(pBuffer, static_cast<size_t>(*pBufferCharacters), spUTF16.GetPtr());
                    nOutputCharacters += nCharacters;
                }

                // find the next list item start index
                while (nListItemStartIndex < pAPETagField->GetFieldValueSize())
                {
                    if (pAPETagField->GetFieldValue()[nListItemStartIndex] == 0)
                    {
                        // found another item
                        nListItemStartIndex += 1; // start after the NULL
                        break;
                    }
                    nListItemStartIndex++;
                }
            }

            // output characters
            if (nResult == ERROR_SUCCESS)
                *pBufferCharacters = nOutputCharacters;
        }
        else
        {
            // memset the whole buffer to NULL (so everything left over is NULL terminated)
            memset(pBuffer, 0, static_cast<size_t>(*pBufferCharacters) * sizeof(pBuffer[0]));

            // do a binary dump (need to convert from wchar's to bytes)
            int nBufferBytes = (*pBufferCharacters - 1) * static_cast<int>(sizeof(pBuffer[0]));
            nResult = GetFieldBinary(pFieldName, pBuffer, &nBufferBytes);
            *pBufferCharacters = (nBufferBytes / static_cast<int>(sizeof(pBuffer[0]))) + 1;
        }
    }

    return nResult;
}

int CAPETag::GetFieldBinary(const str_utfn * pFieldName, void * pBuffer, int * pBufferBytes)
{
    if (!m_bAnalyzed) { Analyze(); }

    int nResult = ERROR_UNDEFINED;

    if (*pBufferBytes > 0)
    {
        CAPETagField * pAPETagField = GetTagField(pFieldName);
        if (pAPETagField == APE_NULL)
        {
            memset(pBuffer, 0, static_cast<size_t>(*pBufferBytes));
            *pBufferBytes = 0;
        }
        else
        {
            if (pAPETagField->GetFieldValueSize() > *pBufferBytes)
            {
                // we'll fail here, because partial data may be worse than no data
                memset(pBuffer, 0, static_cast<size_t>(*pBufferBytes));
                *pBufferBytes = pAPETagField->GetFieldValueSize();
            }
            else
            {
                // memcpy
                *pBufferBytes = pAPETagField->GetFieldValueSize();
                memcpy(pBuffer, pAPETagField->GetFieldValue(), static_cast<size_t>(*pBufferBytes));
                nResult = ERROR_SUCCESS;
            }
        }
    }

    return nResult;
}

int CAPETag::CreateID3Tag(ID3_TAG * pID3Tag)
{
    // error check
    if (pID3Tag == APE_NULL) { return ERROR_UNDEFINED; }
    if (!m_bAnalyzed) { Analyze(); }
    if (m_nFields == 0) { return ERROR_UNDEFINED; }

    // empty
    APE_CLEAR(*pID3Tag);

    // header
    pID3Tag->Header[0] = 'T'; pID3Tag->Header[1] = 'A'; pID3Tag->Header[2] = 'G';

    // standard fields
    GetFieldID3String(APE_TAG_FIELD_ARTIST, pID3Tag->Artist, 30);
    GetFieldID3String(APE_TAG_FIELD_ALBUM, pID3Tag->Album, 30);
    GetFieldID3String(APE_TAG_FIELD_TITLE, pID3Tag->Title, 30);
    GetFieldID3String(APE_TAG_FIELD_COMMENT, pID3Tag->Comment, 29);
    GetFieldID3String(APE_TAG_FIELD_YEAR, pID3Tag->Year, 4);

    // track number
    str_utfn cBuffer[256]; APE_CLEAR(cBuffer); int nBufferCharacters = 255;
    GetFieldString(APE_TAG_FIELD_TRACK, cBuffer, &nBufferCharacters);
    pID3Tag->Track = static_cast<unsigned char>(_wtoi(cBuffer));

    // genre
    cBuffer[0] = 0; nBufferCharacters = 255;
    GetFieldString(APE_TAG_FIELD_GENRE, cBuffer, &nBufferCharacters);

    // convert the genre string to an index
    pID3Tag->Genre = 255;
    int nGenreIndex = 0;
    bool bFound = false;
    while ((nGenreIndex < CAPETag::s_nID3GenreCount) && !bFound)
    {
        if (StringIsEqual(cBuffer, s_aryID3GenreNames[nGenreIndex], false))
        {
            pID3Tag->Genre = static_cast<unsigned char>(nGenreIndex);
            bFound = true;
        }

        nGenreIndex++;
    }

    return ERROR_SUCCESS;
}

bool CAPETag::GetHasID3Tag()
{
    if (!m_bAnalyzed) { Analyze(); }
    return m_bHasID3Tag;
}

bool CAPETag::GetHasAPETag()
{
    if (!m_bAnalyzed) { Analyze(); }
    return m_bHasAPETag;
}

int CAPETag::GetAPETagVersion()
{
    return GetHasAPETag() ? m_nAPETagVersion : -1;
}

void CAPETag::SetIgnoreReadOnly(bool bIgnoreReadOnly)
{
    m_bIgnoreReadOnly = bIgnoreReadOnly;
}

static inline int Load32(const void * a)
{
    return
        (static_cast<int>((static_cast<const unsigned char *>(a))[0]) << 0) |
        (static_cast<int>((static_cast<const unsigned char *>(a))[1]) << 8) |
        (static_cast<int>((static_cast<const unsigned char *>(a))[2]) << 16) |
        (static_cast<int>((static_cast<const unsigned char *>(a))[3]) << 24);
}

int CAPETag::LoadField(const char * pBuffer, int nMaximumBytes, int * pBytes)
{
    // set bytes to 0
    if (pBytes) *pBytes = 0;

    // size and flags
    if (nMaximumBytes < 8)
        return ERROR_UNDEFINED;
    int nLocation = 0;
    const int nFieldValueSize = Load32(&pBuffer[nLocation]);
    if (nFieldValueSize < 1 || nFieldValueSize > (nMaximumBytes - 8))
        return ERROR_UNDEFINED;
    nLocation += 4;
    const int nFieldFlags = Load32(&pBuffer[nLocation]);
    nLocation += 4;

    // safety check (so we can't get buffer overflow attacked)
    bool bSafe = false;
    const int nMaximumRead = nMaximumBytes - 8 - nFieldValueSize;
    if (nMaximumRead > 0)
    {
        for (int z = 0; z < nMaximumRead; z++)
        {
            const int nCharacter = pBuffer[nLocation + z];
            if (nCharacter == 0)
            {
                bSafe = true;
                break;
            }
            if ((nCharacter < 0x20) || (nCharacter > 0x7E))
                break;
        }
    }
    if (!bSafe)
        return ERROR_UNDEFINED;

    // name
    const int nNameCharacters = static_cast<int>(strlen(&pBuffer[nLocation]));
    CSmartPtr<str_utf8> spNameUTF8(new str_utf8 [static_cast<size_t>(nNameCharacters) + 1], true);
    memcpy(spNameUTF8, &pBuffer[nLocation], (static_cast<size_t>(nNameCharacters) + 1) * sizeof(spNameUTF8[0]));
    nLocation += nNameCharacters + 1;
    CSmartPtr<str_utfn> spNameUTF16(CAPECharacterHelper::GetUTF16FromUTF8(spNameUTF8.GetPtr()), true);

    // value
    CSmartPtr<char> spFieldBuffer(new char [static_cast<size_t>(nFieldValueSize)], true);
    memcpy(spFieldBuffer, &pBuffer[nLocation], static_cast<size_t>(nFieldValueSize));
    nLocation += nFieldValueSize;

    // update the bytes
    if (pBytes) *pBytes = nLocation;

    // set
    return SetFieldBinary(spNameUTF16.GetPtr(), spFieldBuffer, nFieldValueSize, nFieldFlags);
}

int CAPETag::SetFieldString(const str_utfn * pFieldName, const str_utfn * pFieldValue, const str_utfn * pListDelimiter)
{
    // remove if empty
    if ((pFieldValue == APE_NULL) || (wcslen(pFieldValue) <= 0))
        return RemoveField(pFieldName);

    // UTF-8 encode the value and call the UTF-8 SetField(...)
    CSmartPtr<str_utf8> spFieldValueUTF8(CAPECharacterHelper::GetUTF8FromUTF16(reinterpret_cast<const str_utfn *>(pFieldValue)), true);
    return SetFieldString(pFieldName, reinterpret_cast<const char *>(spFieldValueUTF8.GetPtr()), true, pListDelimiter);
}

int CAPETag::SetFieldString(const str_utfn * pFieldName, const char * pFieldValue, bool bAlreadyUTF8Encoded, const str_utfn * pListDelimiter)
{
    // remove if empty
    if ((pFieldValue == APE_NULL) || (strlen(pFieldValue) <= 0))
        return RemoveField(pFieldName);

    if (pListDelimiter != APE_NULL)
    {
        // convert from a caller-specified delimiter to NULL delimiting for use in the tag

        // build UTF-8 string
        CSmartPtr<char> spValueUTF8;
        if (bAlreadyUTF8Encoded)
        {
            const intn nCharacters = static_cast<intn>(strlen(pFieldValue)) + 1;
            spValueUTF8.Assign(new char [static_cast<size_t>(nCharacters)]);
            strcpy_s(spValueUTF8, static_cast<size_t>(nCharacters), pFieldValue);
        }
        else
        {
            spValueUTF8.Assign(reinterpret_cast<char *>(CAPECharacterHelper::GetUTF8FromANSI(pFieldValue)), true);
        }

        // get length
        int nValueBytes = static_cast<int>(strlen(static_cast<const char *>(spValueUTF8)));

        // convert from semi-colon delimited to NULL delimited for list fields
        for (int nCharacter = nValueBytes - 1; nCharacter >= 0; nCharacter--)
        {
            if (spValueUTF8[nCharacter] == ';')
            {
                // remove space following delimiter (so we change "a; b; c" to "a;b;c")
                if (spValueUTF8[nCharacter + 1] == ' ')
                {
                    memmove(&spValueUTF8[nCharacter], &spValueUTF8[static_cast<size_t>(nCharacter) + 1], static_cast<size_t>(nValueBytes) - static_cast<size_t>(nCharacter));
                    nValueBytes -= 1;
                }

                // convert from semi-colon to NULL
                spValueUTF8[nCharacter] = 0;
            }
        }

        return SetFieldBinary(pFieldName, spValueUTF8.GetPtr(), nValueBytes, TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8);
    }
    else
    {
        // get the length and call the binary SetField(...)
        if (!bAlreadyUTF8Encoded)
        {
            CSmartPtr<char> spUTF8(reinterpret_cast<char *>(CAPECharacterHelper::GetUTF8FromANSI(pFieldValue)), true);
            const intn nFieldBytes = static_cast<intn>(strlen(spUTF8.GetPtr()));
            return SetFieldBinary(pFieldName, spUTF8.GetPtr(), nFieldBytes, TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8);
        }
        else
        {
            const intn nFieldBytes = static_cast<intn>(strlen(pFieldValue));
            return SetFieldBinary(pFieldName, pFieldValue, nFieldBytes, TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8);
        }
    }
}

int CAPETag::SetFieldBinary(const str_utfn * pFieldName, const void * pFieldValue, intn nFieldBytes, int nFieldFlags)
{
    if (!m_bAnalyzed) { Analyze(); }
    if (pFieldName == APE_NULL) return ERROR_UNDEFINED;
    if (m_nFields >= m_nAllocatedFields)
    {
        const int nOriginalAllocatedFields = m_nAllocatedFields;
        m_nAllocatedFields *= 2;
        m_nAllocatedFields = ape_max(m_nAllocatedFields, 256);
        CAPETagField ** paryFields = new CAPETagField * [static_cast<size_t>(m_nAllocatedFields)];
        if (nOriginalAllocatedFields > 0)
            memcpy(paryFields, m_aryFields, sizeof(paryFields[0]) * static_cast<size_t>(nOriginalAllocatedFields));
        APE_SAFE_ARRAY_DELETE(m_aryFields)
        m_aryFields = paryFields;
    }

    // check to see if we're trying to remove the field (by setting it to NULL or an empty string)
    const bool bRemoving = (pFieldValue == APE_NULL) || (nFieldBytes <= 0);

    // get the index
    int nFieldIndex = GetTagFieldIndex(pFieldName);
    if (nFieldIndex != -1)
    {
        // existing field

        // fail if we're read-only (and not ignoring the read-only flag)
        if (!m_bIgnoreReadOnly && (m_aryFields[nFieldIndex]->GetIsReadOnly()))
            return ERROR_UNDEFINED;

        // erase the existing field
        APE_SAFE_DELETE(m_aryFields[nFieldIndex])

        if (bRemoving)
        {
            return RemoveField(nFieldIndex);
        }
    }
    else
    {
        if (bRemoving)
            return ERROR_SUCCESS;

        nFieldIndex = m_nFields;
        m_nFields++;
    }

    // create the field and add it to the field array
    m_aryFields[nFieldIndex] = new CAPETagField(pFieldName, pFieldValue, static_cast<int>(nFieldBytes), nFieldFlags);

    return ERROR_SUCCESS;
}

int CAPETag::RemoveField(int nIndex)
{
    if ((nIndex >= 0) && (nIndex < m_nFields))
    {
        if (m_aryFields[nIndex] != APE_NULL)
        {
            delete m_aryFields[nIndex];
            m_aryFields[nIndex] = APE_NULL;
        }
        memmove(&m_aryFields[nIndex], &m_aryFields[static_cast<size_t>(nIndex) + 1], (static_cast<size_t>(m_nAllocatedFields) - static_cast<size_t>(nIndex) - 1) * sizeof(m_aryFields[0]));
        m_nFields--;
        return ERROR_SUCCESS;
    }

    return ERROR_UNDEFINED;
}

int CAPETag::RemoveField(const str_utfn * pFieldName)
{
    return RemoveField(GetTagFieldIndex(pFieldName));
}

int CAPETag::Remove(bool bUpdate)
{
    // variables
    unsigned int nBytesRead = 0;
    int nResult = 0;
    const int64 nOriginalPosition = m_spIO->GetPosition();

    bool bID3Removed = true;
    bool bAPETagRemoved = true;

    bool bFailedToRemove = false;

    while (bID3Removed || bAPETagRemoved)
    {
        bID3Removed = false;
        bAPETagRemoved = false;

        // ID3 tag
        if (m_bCheckForID3v1 && (m_spIO->GetSize() > ID3_TAG_BYTES))
        {
            m_spIO->Seek(-ID3_TAG_BYTES, SeekFileEnd);
            char cTagHeader[4];
            nResult = m_spIO->Read(cTagHeader, 3, &nBytesRead);
            cTagHeader[3] = 0; // null terminate
            if ((nResult == ERROR_SUCCESS) && (nBytesRead == 3))
            {
                if (strncmp(cTagHeader, "TAG", 3) == 0)
                {
                    m_spIO->Seek(-ID3_TAG_BYTES, SeekFileEnd);
                    if (m_spIO->SetEOF() != 0)
                        bFailedToRemove = true;
                    else
                        bID3Removed = true;
                }
            }
        }

        // APE Tag
        if (m_spIO->GetSize() > APE_TAG_FOOTER_BYTES && !bFailedToRemove)
        {
            APE_TAG_FOOTER APETagFooter;
            m_spIO->Seek(-static_cast<int>(APE_TAG_FOOTER_BYTES), SeekFileEnd);
            APETagFooter.Empty();
            nResult = m_spIO->Read(&APETagFooter, APE_TAG_FOOTER_BYTES, & nBytesRead);
            if ((nResult == ERROR_SUCCESS) && (nBytesRead == APE_TAG_FOOTER_BYTES))
            {
                if (APETagFooter.GetIsValid(true))
                {
                    m_spIO->Seek(-APETagFooter.GetTotalTagBytes(), SeekFileEnd);

                    if (m_spIO->SetEOF() != 0)
                        bFailedToRemove = true;
                    else
                        bAPETagRemoved = true;
                }
            }
        }
    }

    m_spIO->Seek(nOriginalPosition, SeekFileBegin);

    if (bUpdate && !bFailedToRemove)
    {
        Analyze();
    }

    return bFailedToRemove ? ERROR_UNDEFINED : ERROR_SUCCESS;
}

int CAPETag::SetFieldID3String(const str_utfn * pFieldName, const char * pFieldValue, int nBytes)
{
    // allocate a buffer and terminate it
    CSmartPtr<str_ansi> spBuffer(new str_ansi [static_cast<size_t>(nBytes) + 1], true);
    spBuffer[nBytes] = 0;

    // make a capped copy of the string
    memcpy(spBuffer.GetPtr(), pFieldValue, static_cast<size_t>(nBytes));

    // remove trailing white-space
    char * pEnd = &spBuffer[nBytes];
    while ((pEnd >= &spBuffer[0]) && ((*pEnd == ' ') || (*pEnd == 0))) { *pEnd-- = 0; }

    // set the field
    SetFieldString(pFieldName, spBuffer, false);

    return ERROR_SUCCESS;
}

int CAPETag::GetFieldID3String(const str_utfn * pFieldName, char * pBuffer, int nBytes)
{
    int nBufferCharacters = 255; str_utfn cBuffer[256]; APE_CLEAR(cBuffer);
    GetFieldString(pFieldName, cBuffer, &nBufferCharacters);

    CSmartPtr<str_ansi> spBufferANSI(CAPECharacterHelper::GetANSIFromUTF16(cBuffer), true);

    memset(pBuffer, 0, static_cast<size_t>(nBytes));

    int nStringLength = static_cast<int>(strlen(spBufferANSI.GetPtr()));
    nStringLength = ape_min(nStringLength, nBytes);
    memcpy(pBuffer, spBufferANSI.GetPtr(), static_cast<size_t>(nStringLength));

    return ERROR_SUCCESS;
}

int CAPETag::SortFields()
{
    // sort the tag fields by size (so that the smallest fields are at the front of the tag)
    if (m_nFields > 1)
        qsort(m_aryFields, static_cast<size_t>(m_nFields), sizeof(m_aryFields[0]), CompareFields);

    return ERROR_SUCCESS;
}

int CAPETag::CompareFields(const void * pA, const void * pB)
{
    const CAPETagField * pFieldA = *static_cast<const CAPETagField * const *>(pA);
    const CAPETagField * pFieldB = *static_cast<const CAPETagField * const *>(pB);

    int nValue = (pFieldA->GetFieldSize() - pFieldB->GetFieldSize());
    if (nValue == 0)
    {
        nValue = _wcsicmp(pFieldA->GetFieldName(), pFieldB->GetFieldName());
    }

    return nValue;
}

}
