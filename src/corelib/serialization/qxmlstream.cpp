/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtCore/qxmlstream.h"

#ifndef QT_NO_XMLSTREAM

#include "qxmlutils_p.h"
#include <qdebug.h>
#include <qfile.h>
#include <stdio.h>
#include <qstringconverter.h>
#include <qstack.h>
#include <qbuffer.h>
#include <qscopeguard.h>
#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#else
// This specialization of Q_DECLARE_TR_FUNCTIONS is not in qcoreapplication.h,
// because that header depends on QObject being available, which is not the
// case for most bootstrapped applications.
#define Q_DECLARE_TR_FUNCTIONS(context) \
public: \
    static inline QString tr(const char *sourceText, const char *comment = nullptr) \
        { Q_UNUSED(comment); return QString::fromUtf8(sourceText); } \
    static inline QString tr(const char *sourceText, const char*, int) \
        { return QString::fromUtf8(sourceText); } \
private:
#endif

#include <iterator>
#include "qxmlstream_p.h"
#include "qxmlstreamparser_p.h"

QT_BEGIN_NAMESPACE

using namespace QtPrivate;

enum { StreamEOF = ~0U };

namespace {
template <typename Range>
auto reversed(Range &r)
{
    struct R {
        Range *r;
        auto begin() { return std::make_reverse_iterator(std::end(*r)); }
        auto end() { return std::make_reverse_iterator(std::begin(*r)); }
    };

    return R{&r};
}

template <typename Range>
void reversed(const Range &&) = delete;
}

/*!
    \enum QXmlStreamReader::TokenType

    This enum specifies the type of token the reader just read.

    \value NoToken The reader has not yet read anything.

    \value Invalid An error has occurred, reported in error() and
    errorString().

    \value StartDocument The reader reports the XML version number in
    documentVersion(), and the encoding as specified in the XML
    document in documentEncoding().  If the document is declared
    standalone, isStandaloneDocument() returns \c true; otherwise it
    returns \c false.

    \value EndDocument The reader reports the end of the document.

    \value StartElement The reader reports the start of an element
    with namespaceUri() and name(). Empty elements are also reported
    as StartElement, followed directly by EndElement. The convenience
    function readElementText() can be called to concatenate all
    content until the corresponding EndElement. Attributes are
    reported in attributes(), namespace declarations in
    namespaceDeclarations().

    \value EndElement The reader reports the end of an element with
    namespaceUri() and name().

    \value Characters The reader reports characters in text(). If the
    characters are all white-space, isWhitespace() returns \c true. If
    the characters stem from a CDATA section, isCDATA() returns \c true.

    \value Comment The reader reports a comment in text().

    \value DTD The reader reports a DTD in text(), notation
    declarations in notationDeclarations(), and entity declarations in
    entityDeclarations(). Details of the DTD declaration are reported
    in in dtdName(), dtdPublicId(), and dtdSystemId().

    \value EntityReference The reader reports an entity reference that
    could not be resolved.  The name of the reference is reported in
    name(), the replacement text in text().

    \value ProcessingInstruction The reader reports a processing
    instruction in processingInstructionTarget() and
    processingInstructionData().
*/

/*!
    \enum QXmlStreamReader::ReadElementTextBehaviour

    This enum specifies the different behaviours of readElementText().

    \value ErrorOnUnexpectedElement Raise an UnexpectedElementError and return
    what was read so far when a child element is encountered.

    \value IncludeChildElements Recursively include the text from child elements.

    \value SkipChildElements Skip child elements.

    \since 4.6
*/

/*!
    \enum QXmlStreamReader::Error

    This enum specifies different error cases

    \value NoError No error has occurred.

    \value CustomError A custom error has been raised with
    raiseError()

    \value NotWellFormedError The parser internally raised an error
    due to the read XML not being well-formed.

    \value PrematureEndOfDocumentError The input stream ended before a
    well-formed XML document was parsed. Recovery from this error is
    possible if more XML arrives in the stream, either by calling
    addData() or by waiting for it to arrive on the device().

    \value UnexpectedElementError The parser encountered an element
    that was different to those it expected.

*/

/*!
  \class QXmlStreamEntityResolver
  \inmodule QtCore
  \reentrant
  \since 4.4

  \brief The QXmlStreamEntityResolver class provides an entity
  resolver for a QXmlStreamReader.

  \ingroup xml-tools
 */

/*!
  Destroys the entity resolver.
 */
QXmlStreamEntityResolver::~QXmlStreamEntityResolver()
{
}

/*!
  \internal

This function is a stub for later functionality.
*/
QString QXmlStreamEntityResolver::resolveEntity(const QString& /*publicId*/, const QString& /*systemId*/)
{
    return QString();
}


/*!
  Resolves the undeclared entity \a name and returns its replacement
  text. If the entity is also unknown to the entity resolver, it
  returns an empty string.

  The default implementation always returns an empty string.
*/

QString QXmlStreamEntityResolver::resolveUndeclaredEntity(const QString &/*name*/)
{
    return QString();
}

#ifndef QT_NO_XMLSTREAMREADER

QString QXmlStreamReaderPrivate::resolveUndeclaredEntity(const QString &name)
{
    if (entityResolver)
        return entityResolver->resolveUndeclaredEntity(name);
    return QString();
}



/*!
   \since 4.4

   Makes \a resolver the new entityResolver().

   The stream reader does \e not take ownership of the resolver. It's
   the callers responsibility to ensure that the resolver is valid
   during the entire life-time of the stream reader object, or until
   another resolver or \nullptr is set.

   \sa entityResolver()
 */
void QXmlStreamReader::setEntityResolver(QXmlStreamEntityResolver *resolver)
{
    Q_D(QXmlStreamReader);
    d->entityResolver = resolver;
}

/*!
  \since 4.4

  Returns the entity resolver, or \nullptr if there is no entity resolver.

  \sa setEntityResolver()
 */
QXmlStreamEntityResolver *QXmlStreamReader::entityResolver() const
{
    Q_D(const QXmlStreamReader);
    return d->entityResolver;
}



/*!
  \class QXmlStreamReader
  \inmodule QtCore
  \reentrant
  \since 4.3

  \brief The QXmlStreamReader class provides a fast parser for reading
  well-formed XML via a simple streaming API.


  \ingroup xml-tools

  QXmlStreamReader provides a simple streaming API to parse well-formed
  XML. It is an alternative to first loading the complete XML into a
  DOM tree (see \l QDomDocument). QXmlStreamReader reads data either
  from a QIODevice (see setDevice()), or from a raw QByteArray (see addData()).

  Qt provides QXmlStreamWriter for writing XML.

  The basic concept of a stream reader is to report an XML document as
  a stream of tokens, similar to SAX. The main difference between
  QXmlStreamReader and SAX is \e how these XML tokens are reported.
  With SAX, the application must provide handlers (callback functions)
  that receive so-called XML \e events from the parser at the parser's
  convenience.  With QXmlStreamReader, the application code itself
  drives the loop and pulls \e tokens from the reader, one after
  another, as it needs them. This is done by calling readNext(), where
  the reader reads from the input stream until it completes the next
  token, at which point it returns the tokenType(). A set of
  convenient functions including isStartElement() and text() can then
  be used to examine the token to obtain information about what has
  been read. The big advantage of this \e pulling approach is the
  possibility to build recursive descent parsers with it, meaning you
  can split your XML parsing code easily into different methods or
  classes. This makes it easy to keep track of the application's own
  state when parsing XML.

  A typical loop with QXmlStreamReader looks like this:

  \snippet code/src_corelib_xml_qxmlstream.cpp 0


  QXmlStreamReader is a well-formed XML 1.0 parser that does \e not
  include external parsed entities. As long as no error occurs, the
  application code can thus be assured that the data provided by the
  stream reader satisfies the W3C's criteria for well-formed XML. For
  example, you can be certain that all tags are indeed nested and
  closed properly, that references to internal entities have been
  replaced with the correct replacement text, and that attributes have
  been normalized or added according to the internal subset of the
  DTD.

  If an error occurs while parsing, atEnd() and hasError() return
  true, and error() returns the error that occurred. The functions
  errorString(), lineNumber(), columnNumber(), and characterOffset()
  are for constructing an appropriate error or warning message. To
  simplify application code, QXmlStreamReader contains a raiseError()
  mechanism that lets you raise custom errors that trigger the same
  error handling described.

  The \l{QXmlStream Bookmarks Example} illustrates how to use the
  recursive descent technique to read an XML bookmark file (XBEL) with
  a stream reader.

  \section1 Namespaces

  QXmlStream understands and resolves XML namespaces. E.g. in case of
  a StartElement, namespaceUri() returns the namespace the element is
  in, and name() returns the element's \e local name. The combination
  of namespaceUri and name uniquely identifies an element. If a
  namespace prefix was not declared in the XML entities parsed by the
  reader, the namespaceUri is empty.

  If you parse XML data that does not utilize namespaces according to
  the XML specification or doesn't use namespaces at all, you can use
  the element's qualifiedName() instead. A qualified name is the
  element's prefix() followed by colon followed by the element's local
  name() - exactly like the element appears in the raw XML data. Since
  the mapping namespaceUri to prefix is neither unique nor universal,
  qualifiedName() should be avoided for namespace-compliant XML data.

  In order to parse standalone documents that do use undeclared
  namespace prefixes, you can turn off namespace processing completely
  with the \l namespaceProcessing property.

  \section1 Incremental Parsing

  QXmlStreamReader is an incremental parser. It can handle the case
  where the document can't be parsed all at once because it arrives in
  chunks (e.g. from multiple files, or over a network connection).
  When the reader runs out of data before the complete document has
  been parsed, it reports a PrematureEndOfDocumentError. When more
  data arrives, either because of a call to addData() or because more
  data is available through the network device(), the reader recovers
  from the PrematureEndOfDocumentError error and continues parsing the
  new data with the next call to readNext().

  For example, if your application reads data from the network using a
  \l{QNetworkAccessManager} {network access manager}, you would issue
  a \l{QNetworkRequest} {network request} to the manager and receive a
  \l{QNetworkReply} {network reply} in return. Since a QNetworkReply
  is a QIODevice, you connect its \l{QIODevice::readyRead()}
  {readyRead()} signal to a custom slot, e.g. \c{slotReadyRead()} in
  the code snippet shown in the discussion for QNetworkAccessManager.
  In this slot, you read all available data with
  \l{QIODevice::readAll()} {readAll()} and pass it to the XML
  stream reader using addData(). Then you call your custom parsing
  function that reads the XML events from the reader.

  \section1 Performance and Memory Consumption

  QXmlStreamReader is memory-conservative by design, since it doesn't
  store the entire XML document tree in memory, but only the current
  token at the time it is reported. In addition, QXmlStreamReader
  avoids the many small string allocations that it normally takes to
  map an XML document to a convenient and Qt-ish API. It does this by
  reporting all string data as QStringView rather than real QString
  objects. Calling \l{QStringView::toString()}{toString()} on any of
  those objects returns an equivalent real QString object.
*/


/*!
  Constructs a stream reader.

  \sa setDevice(), addData()
 */
QXmlStreamReader::QXmlStreamReader()
    : d_ptr(new QXmlStreamReaderPrivate(this))
{
}

/*!  Creates a new stream reader that reads from \a device.

\sa setDevice(), clear()
 */
QXmlStreamReader::QXmlStreamReader(QIODevice *device)
    : d_ptr(new QXmlStreamReaderPrivate(this))
{
    setDevice(device);
}

/*!
  Creates a new stream reader that reads from \a data.

  \sa addData(), clear(), setDevice()
 */
QXmlStreamReader::QXmlStreamReader(const QByteArray &data)
    : d_ptr(new QXmlStreamReaderPrivate(this))
{
    Q_D(QXmlStreamReader);
    d->dataBuffer = data;
}

/*!
  Creates a new stream reader that reads from \a data.

  \sa addData(), clear(), setDevice()
 */
QXmlStreamReader::QXmlStreamReader(const QString &data)
    : d_ptr(new QXmlStreamReaderPrivate(this))
{
    Q_D(QXmlStreamReader);
    d->dataBuffer = data.toUtf8();
    d->decoder = QStringDecoder(QStringDecoder::Utf8);
    d->lockEncoding = true;
}

/*!
  Creates a new stream reader that reads from \a data.

  \sa addData(), clear(), setDevice()
 */
QXmlStreamReader::QXmlStreamReader(const char *data)
    : d_ptr(new QXmlStreamReaderPrivate(this))
{
    Q_D(QXmlStreamReader);
    d->dataBuffer = QByteArray(data);
}

/*!
  Destructs the reader.
 */
QXmlStreamReader::~QXmlStreamReader()
{
    Q_D(QXmlStreamReader);
    if (d->deleteDevice)
        delete d->device;
}

/*! \fn bool QXmlStreamReader::hasError() const
    Returns \c true if an error has occurred, otherwise \c false.

    \sa errorString(), error()
 */

/*!
    Sets the current device to \a device. Setting the device resets
    the stream to its initial state.

    \sa device(), clear()
*/
void QXmlStreamReader::setDevice(QIODevice *device)
{
    Q_D(QXmlStreamReader);
    if (d->deleteDevice) {
        delete d->device;
        d->deleteDevice = false;
    }
    d->device = device;
    d->init();

}

/*!
    Returns the current device associated with the QXmlStreamReader,
    or \nullptr if no device has been assigned.

    \sa setDevice()
*/
QIODevice *QXmlStreamReader::device() const
{
    Q_D(const QXmlStreamReader);
    return d->device;
}


/*!
  Adds more \a data for the reader to read. This function does
  nothing if the reader has a device().

  \sa readNext(), clear()
 */
void QXmlStreamReader::addData(const QByteArray &data)
{
    Q_D(QXmlStreamReader);
    if (d->device) {
        qWarning("QXmlStreamReader: addData() with device()");
        return;
    }
    d->dataBuffer += data;
}

/*!
  Adds more \a data for the reader to read. This function does
  nothing if the reader has a device().

  \sa readNext(), clear()
 */
void QXmlStreamReader::addData(const QString &data)
{
    Q_D(QXmlStreamReader);
    d->lockEncoding = true;
    if (!d->decoder.isValid())
        d->decoder = QStringDecoder(QStringDecoder::Utf8);
    addData(data.toUtf8());
}

/*!
  Adds more \a data for the reader to read. This function does
  nothing if the reader has a device().

  \sa readNext(), clear()
 */
void QXmlStreamReader::addData(const char *data)
{
    addData(QByteArray(data));
}

/*!
    Removes any device() or data from the reader and resets its
    internal state to the initial state.

    \sa addData()
 */
void QXmlStreamReader::clear()
{
    Q_D(QXmlStreamReader);
    d->init();
    if (d->device) {
        if (d->deleteDevice)
            delete d->device;
        d->device = nullptr;
    }
}

/*!
    Returns \c true if the reader has read until the end of the XML
    document, or if an error() has occurred and reading has been
    aborted. Otherwise, it returns \c false.

    When atEnd() and hasError() return true and error() returns
    PrematureEndOfDocumentError, it means the XML has been well-formed
    so far, but a complete XML document has not been parsed. The next
    chunk of XML can be added with addData(), if the XML is being read
    from a QByteArray, or by waiting for more data to arrive if the
    XML is being read from a QIODevice. Either way, atEnd() will
    return false once more data is available.

    \sa hasError(), error(), device(), QIODevice::atEnd()
 */
bool QXmlStreamReader::atEnd() const
{
    Q_D(const QXmlStreamReader);
    if (d->atEnd
        && ((d->type == QXmlStreamReader::Invalid && d->error == PrematureEndOfDocumentError)
            || (d->type == QXmlStreamReader::EndDocument))) {
        if (d->device)
            return d->device->atEnd();
        else
            return !d->dataBuffer.size();
    }
    return (d->atEnd || d->type == QXmlStreamReader::Invalid);
}


/*!
  Reads the next token and returns its type.

  With one exception, once an error() is reported by readNext(),
  further reading of the XML stream is not possible. Then atEnd()
  returns \c true, hasError() returns \c true, and this function returns
  QXmlStreamReader::Invalid.

  The exception is when error() returns PrematureEndOfDocumentError.
  This error is reported when the end of an otherwise well-formed
  chunk of XML is reached, but the chunk doesn't represent a complete
  XML document.  In that case, parsing \e can be resumed by calling
  addData() to add the next chunk of XML, when the stream is being
  read from a QByteArray, or by waiting for more data to arrive when
  the stream is being read from a device().

  \sa tokenType(), tokenString()
 */
QXmlStreamReader::TokenType QXmlStreamReader::readNext()
{
    Q_D(QXmlStreamReader);
    if (d->type != Invalid) {
        if (!d->hasCheckedStartDocument)
            if (!d->checkStartDocument())
                return d->type; // synthetic StartDocument or error
        d->parse();
        if (d->atEnd && d->type != EndDocument && d->type != Invalid)
            d->raiseError(PrematureEndOfDocumentError);
        else if (!d->atEnd && d->type == EndDocument)
            d->raiseWellFormedError(QXmlStream::tr("Extra content at end of document."));
    } else if (d->error == PrematureEndOfDocumentError) {
        // resume error
        d->type = NoToken;
        d->atEnd = false;
        d->token = -1;
        return readNext();
    }
    return d->type;
}


/*!
  Returns the type of the current token.

  The current token can also be queried with the convenience functions
  isStartDocument(), isEndDocument(), isStartElement(),
  isEndElement(), isCharacters(), isComment(), isDTD(),
  isEntityReference(), and isProcessingInstruction().

  \sa tokenString()
 */
QXmlStreamReader::TokenType QXmlStreamReader::tokenType() const
{
    Q_D(const QXmlStreamReader);
    return d->type;
}

/*!
  Reads until the next start element within the current element. Returns \c true
  when a start element was reached. When the end element was reached, or when
  an error occurred, false is returned.

  The current element is the element matching the most recently parsed start
  element of which a matching end element has not yet been reached. When the
  parser has reached the end element, the current element becomes the parent
  element.

  This is a convenience function for when you're only concerned with parsing
  XML elements. The \l{QXmlStream Bookmarks Example} makes extensive use of
  this function.

  \since 4.6
  \sa readNext()
 */
bool QXmlStreamReader::readNextStartElement()
{
    while (readNext() != Invalid) {
        if (isEndElement())
            return false;
        else if (isStartElement())
            return true;
    }
    return false;
}

/*!
  Reads until the end of the current element, skipping any child nodes.
  This function is useful for skipping unknown elements.

  The current element is the element matching the most recently parsed start
  element of which a matching end element has not yet been reached. When the
  parser has reached the end element, the current element becomes the parent
  element.

  \since 4.6
 */
void QXmlStreamReader::skipCurrentElement()
{
    int depth = 1;
    while (depth && readNext() != Invalid) {
        if (isEndElement())
            --depth;
        else if (isStartElement())
            ++depth;
    }
}

/*
 * Use the following Perl script to generate the error string index list:
===== PERL SCRIPT ====
print "static const char QXmlStreamReader_tokenTypeString_string[] =\n";
$counter = 0;
$i = 0;
while (<STDIN>) {
    chomp;
    print "    \"$_\\0\"\n";
    $sizes[$i++] = $counter;
    $counter += length 1 + $_;
}
print "    \"\\0\";\n\nstatic const short QXmlStreamReader_tokenTypeString_indices[] = {\n    ";
for ($j = 0; $j < $i; ++$j) {
    printf "$sizes[$j], ";
}
print "0\n};\n";
===== PERL SCRIPT ====

 * The input data is as follows (copied from qxmlstream.h):
NoToken
Invalid
StartDocument
EndDocument
StartElement
EndElement
Characters
Comment
DTD
EntityReference
ProcessingInstruction
*/
static const char QXmlStreamReader_tokenTypeString_string[] =
    "NoToken\0"
    "Invalid\0"
    "StartDocument\0"
    "EndDocument\0"
    "StartElement\0"
    "EndElement\0"
    "Characters\0"
    "Comment\0"
    "DTD\0"
    "EntityReference\0"
    "ProcessingInstruction\0";

static const short QXmlStreamReader_tokenTypeString_indices[] = {
    0, 8, 16, 30, 42, 55, 66, 77, 85, 89, 105, 0
};


/*!
    \property  QXmlStreamReader::namespaceProcessing
    \brief the namespace-processing flag of the stream reader.

    This property controls whether or not the stream reader processes
    namespaces. If enabled, the reader processes namespaces, otherwise
    it does not.

    By default, namespace-processing is enabled.
*/


void QXmlStreamReader::setNamespaceProcessing(bool enable)
{
    Q_D(QXmlStreamReader);
    d->namespaceProcessing = enable;
}

bool QXmlStreamReader::namespaceProcessing() const
{
    Q_D(const QXmlStreamReader);
    return d->namespaceProcessing;
}

/*! Returns the reader's current token as string.

\sa tokenType()
*/
QString QXmlStreamReader::tokenString() const
{
    Q_D(const QXmlStreamReader);
    return QLatin1String(QXmlStreamReader_tokenTypeString_string +
                         QXmlStreamReader_tokenTypeString_indices[d->type]);
}

#endif // QT_NO_XMLSTREAMREADER

QXmlStreamPrivateTagStack::QXmlStreamPrivateTagStack()
{
    tagStack.reserve(16);
    tagStackStringStorage.reserve(32);
    tagStackStringStorageSize = 0;
    NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
    namespaceDeclaration.prefix = addToStringStorage(u"xml");
    namespaceDeclaration.namespaceUri = addToStringStorage(u"http://www.w3.org/XML/1998/namespace");
    initialTagStackStringStorageSize = tagStackStringStorageSize;
    tagsDone = false;
}

#ifndef QT_NO_XMLSTREAMREADER

QXmlStreamReaderPrivate::QXmlStreamReaderPrivate(QXmlStreamReader *q)
    :q_ptr(q)
{
    device = nullptr;
    deleteDevice = false;
    stack_size = 64;
    sym_stack = nullptr;
    state_stack = nullptr;
    reallocateStack();
    entityResolver = nullptr;
    init();
#define ADD_PREDEFINED(n, v) \
    do { \
        Entity e = Entity::createLiteral(QLatin1String(n), QLatin1String(v)); \
        entityHash.insert(qToStringViewIgnoringNull(e.name), std::move(e)); \
    } while (false)
    ADD_PREDEFINED("lt", "<");
    ADD_PREDEFINED("gt", ">");
    ADD_PREDEFINED("amp", "&");
    ADD_PREDEFINED("apos", "'");
    ADD_PREDEFINED("quot", "\"");
#undef ADD_PREDEFINED
}

void QXmlStreamReaderPrivate::init()
{
    scanDtd = false;
    lastAttributeIsCData = false;
    token = -1;
    token_char = 0;
    isEmptyElement = false;
    isWhitespace = true;
    isCDATA = false;
    standalone = false;
    tos = 0;
    resumeReduction = 0;
    state_stack[tos++] = 0;
    state_stack[tos] = 0;
    putStack.clear();
    putStack.reserve(32);
    textBuffer.clear();
    textBuffer.reserve(256);
    tagStack.clear();
    tagsDone = false;
    attributes.clear();
    attributes.reserve(16);
    lineNumber = lastLineStart = characterOffset = 0;
    readBufferPos = 0;
    nbytesread = 0;
    decoder = QStringDecoder();
    attributeStack.clear();
    attributeStack.reserve(16);
    entityParser.reset();
    hasCheckedStartDocument = false;
    normalizeLiterals = false;
    hasSeenTag = false;
    atEnd = false;
    inParseEntity = false;
    referenceToUnparsedEntityDetected = false;
    referenceToParameterEntityDetected = false;
    hasExternalDtdSubset = false;
    lockEncoding = false;
    namespaceProcessing = true;
    rawReadBuffer.clear();
    dataBuffer.clear();
    readBuffer.clear();
    tagStackStringStorageSize = initialTagStackStringStorageSize;

    type = QXmlStreamReader::NoToken;
    error = QXmlStreamReader::NoError;
}

/*
  Well-formed requires that we verify entity values. We do this with a
  standard parser.
 */
void QXmlStreamReaderPrivate::parseEntity(const QString &value)
{
    Q_Q(QXmlStreamReader);

    if (value.isEmpty())
        return;


    if (!entityParser)
        entityParser = std::make_unique<QXmlStreamReaderPrivate>(q);
    else
        entityParser->init();
    entityParser->inParseEntity = true;
    entityParser->readBuffer = value;
    entityParser->injectToken(PARSE_ENTITY);
    while (!entityParser->atEnd && entityParser->type != QXmlStreamReader::Invalid)
        entityParser->parse();
    if (entityParser->type == QXmlStreamReader::Invalid || entityParser->tagStack.size())
        raiseWellFormedError(QXmlStream::tr("Invalid entity value."));

}

inline void QXmlStreamReaderPrivate::reallocateStack()
{
    stack_size <<= 1;
    sym_stack = reinterpret_cast<Value*> (realloc(sym_stack, stack_size * sizeof(Value)));
    Q_CHECK_PTR(sym_stack);
    state_stack = reinterpret_cast<int*> (realloc(state_stack, stack_size * sizeof(int)));
    Q_CHECK_PTR(state_stack);
}


QXmlStreamReaderPrivate::~QXmlStreamReaderPrivate()
{
    free(sym_stack);
    free(state_stack);
}


inline uint QXmlStreamReaderPrivate::filterCarriageReturn()
{
    uint peekc = peekChar();
    if (peekc == '\n') {
        if (putStack.size())
            putStack.pop();
        else
            ++readBufferPos;
        return peekc;
    }
    if (peekc == StreamEOF) {
        putChar('\r');
        return 0;
    }
    return '\n';
}

/*!
 \internal
 If the end of the file is encountered, ~0 is returned.
 */
inline uint QXmlStreamReaderPrivate::getChar()
{
    uint c;
    if (putStack.size()) {
        c = atEnd ? StreamEOF : putStack.pop();
    } else {
        if (readBufferPos < readBuffer.size())
            c = readBuffer.at(readBufferPos++).unicode();
        else
            c = getChar_helper();
    }

    return c;
}

inline uint QXmlStreamReaderPrivate::peekChar()
{
    uint c;
    if (putStack.size()) {
        c = putStack.top();
    } else if (readBufferPos < readBuffer.size()) {
        c = readBuffer.at(readBufferPos).unicode();
    } else {
        if ((c = getChar_helper()) != StreamEOF)
            --readBufferPos;
    }

    return c;
}

/*!
  \internal

  Scans characters until \a str is encountered, and validates the characters
  as according to the Char[2] production and do the line-ending normalization.
  If any character is invalid, false is returned, otherwise true upon success.

  If \a tokenToInject is not less than zero, injectToken() is called with
  \a tokenToInject when \a str is found.

  If any error occurred, false is returned, otherwise true.
  */
bool QXmlStreamReaderPrivate::scanUntil(const char *str, short tokenToInject)
{
    const qsizetype pos = textBuffer.size();
    const auto oldLineNumber = lineNumber;

    uint c;
    while ((c = getChar()) != StreamEOF) {
        /* First, we do the validation & normalization. */
        switch (c) {
        case '\r':
            if ((c = filterCarriageReturn()) == 0)
                break;
            Q_FALLTHROUGH();
        case '\n':
            ++lineNumber;
            lastLineStart = characterOffset + readBufferPos;
            Q_FALLTHROUGH();
        case '\t':
            textBuffer += QChar(c);
            continue;
        default:
            if (c < 0x20 || (c > 0xFFFD && c < 0x10000) || c > QChar::LastValidCodePoint ) {
                raiseWellFormedError(QXmlStream::tr("Invalid XML character."));
                lineNumber = oldLineNumber;
                return false;
            }
            textBuffer += QChar(c);
        }


        /* Second, attempt to lookup str. */
        if (c == uint(*str)) {
            if (!*(str + 1)) {
                if (tokenToInject >= 0)
                    injectToken(tokenToInject);
                return true;
            } else {
                if (scanString(str + 1, tokenToInject, false))
                    return true;
            }
        }
    }
    putString(textBuffer, pos);
    textBuffer.resize(pos);
    lineNumber = oldLineNumber;
    return false;
}

bool QXmlStreamReaderPrivate::scanString(const char *str, short tokenToInject, bool requireSpace)
{
    qsizetype n = 0;
    while (str[n]) {
        uint c = getChar();
        if (c != ushort(str[n])) {
            if (c != StreamEOF)
                putChar(c);
            while (n--) {
                putChar(ushort(str[n]));
            }
            return false;
        }
        ++n;
    }
    textBuffer += QLatin1String(str, n);
    if (requireSpace) {
        int s = fastScanSpace();
        if (!s || atEnd) {
            qsizetype pos = textBuffer.size() - n - s;
            putString(textBuffer, pos);
            textBuffer.resize(pos);
            return false;
        }
    }
    if (tokenToInject >= 0)
        injectToken(tokenToInject);
    return true;
}

bool QXmlStreamReaderPrivate::scanAfterLangleBang()
{
    switch (peekChar()) {
    case '[':
        return scanString(spell[CDATA_START], CDATA_START, false);
    case 'D':
        return scanString(spell[DOCTYPE], DOCTYPE);
    case 'A':
        return scanString(spell[ATTLIST], ATTLIST);
    case 'N':
        return scanString(spell[NOTATION], NOTATION);
    case 'E':
        if (scanString(spell[ELEMENT], ELEMENT))
            return true;
        return scanString(spell[ENTITY], ENTITY);

    default:
        ;
    };
    return false;
}

bool QXmlStreamReaderPrivate::scanPublicOrSystem()
{
    switch (peekChar()) {
    case 'S':
        return scanString(spell[SYSTEM], SYSTEM);
    case 'P':
        return scanString(spell[PUBLIC], PUBLIC);
    default:
        ;
    }
    return false;
}

bool QXmlStreamReaderPrivate::scanNData()
{
    if (fastScanSpace()) {
        if (scanString(spell[NDATA], NDATA))
            return true;
        putChar(' ');
    }
    return false;
}

bool QXmlStreamReaderPrivate::scanAfterDefaultDecl()
{
    switch (peekChar()) {
    case 'R':
        return scanString(spell[REQUIRED], REQUIRED, false);
    case 'I':
        return scanString(spell[IMPLIED], IMPLIED, false);
    case 'F':
        return scanString(spell[FIXED], FIXED, false);
    default:
        ;
    }
    return false;
}

bool QXmlStreamReaderPrivate::scanAttType()
{
    switch (peekChar()) {
    case 'C':
        return scanString(spell[CDATA], CDATA);
    case 'I':
        if (scanString(spell[ID], ID))
            return true;
        if (scanString(spell[IDREF], IDREF))
            return true;
        return scanString(spell[IDREFS], IDREFS);
    case 'E':
        if (scanString(spell[ENTITY], ENTITY))
            return true;
        return scanString(spell[ENTITIES], ENTITIES);
    case 'N':
        if (scanString(spell[NOTATION], NOTATION))
            return true;
        if (scanString(spell[NMTOKEN], NMTOKEN))
            return true;
        return scanString(spell[NMTOKENS], NMTOKENS);
    default:
        ;
    }
    return false;
}

/*!
 \internal

 Scan strings with quotes or apostrophes surround them. For instance,
 attributes, the version and encoding field in the XML prolog and
 entity declarations.

 If normalizeLiterals is set to true, the function also normalizes
 whitespace. It is set to true when the first start tag is
 encountered.

 */
inline int QXmlStreamReaderPrivate::fastScanLiteralContent()
{
    int n = 0;
    uint c;
    while ((c = getChar()) != StreamEOF) {
        switch (ushort(c)) {
        case 0xfffe:
        case 0xffff:
        case 0:
            /* The putChar() call is necessary so the parser re-gets
             * the character from the input source, when raising an error. */
            putChar(c);
            return n;
        case '\r':
            if (filterCarriageReturn() == 0)
                return n;
            Q_FALLTHROUGH();
        case '\n':
            ++lineNumber;
            lastLineStart = characterOffset + readBufferPos;
            Q_FALLTHROUGH();
        case ' ':
        case '\t':
            if (normalizeLiterals)
                textBuffer += QLatin1Char(' ');
            else
                textBuffer += QChar(c);
            ++n;
            break;
        case '&':
        case '<':
        case '\"':
        case '\'':
            if (!(c & 0xff0000)) {
                putChar(c);
                return n;
            }
            Q_FALLTHROUGH();
        default:
            if (c < 0x20) {
                putChar(c);
                return n;
            }
            textBuffer += QChar(ushort(c));
            ++n;
        }
    }
    return n;
}

inline int QXmlStreamReaderPrivate::fastScanSpace()
{
    int n = 0;
    uint c;
    while ((c = getChar()) != StreamEOF) {
        switch (c) {
        case '\r':
            if ((c = filterCarriageReturn()) == 0)
                return n;
            Q_FALLTHROUGH();
        case '\n':
            ++lineNumber;
            lastLineStart = characterOffset + readBufferPos;
            Q_FALLTHROUGH();
        case ' ':
        case '\t':
            textBuffer += QChar(c);
            ++n;
            break;
        default:
            putChar(c);
            return n;
        }
    }
    return n;
}

/*!
  \internal

  Used for text nodes essentially. That is, characters appearing
  inside elements.
 */
inline int QXmlStreamReaderPrivate::fastScanContentCharList()
{
    int n = 0;
    uint c;
    while ((c = getChar()) != StreamEOF) {
        switch (ushort(c)) {
        case 0xfffe:
        case 0xffff:
        case 0:
            putChar(c);
            return n;
        case ']': {
            isWhitespace = false;
            const qsizetype pos = textBuffer.size();
            textBuffer += QChar(ushort(c));
            ++n;
            while ((c = getChar()) == ']') {
                textBuffer += QChar(ushort(c));
                ++n;
            }
            if (c == 0) {
                putString(textBuffer, pos);
                textBuffer.resize(pos);
            } else if (c == '>' && textBuffer.at(textBuffer.size()-2) == QLatin1Char(']')) {
                raiseWellFormedError(QXmlStream::tr("Sequence ']]>' not allowed in content."));
            } else {
                putChar(c);
                break;
            }
            return n;
        } break;
        case '\r':
            if ((c = filterCarriageReturn()) == 0)
                return n;
            Q_FALLTHROUGH();
        case '\n':
            ++lineNumber;
            lastLineStart = characterOffset + readBufferPos;
            Q_FALLTHROUGH();
        case ' ':
        case '\t':
            textBuffer += QChar(ushort(c));
            ++n;
            break;
        case '&':
        case '<':
            if (!(c & 0xff0000)) {
                putChar(c);
                return n;
            }
            Q_FALLTHROUGH();
        default:
            if (c < 0x20) {
                putChar(c);
                return n;
            }
            isWhitespace = false;
            textBuffer += QChar(ushort(c));
            ++n;
        }
    }
    return n;
}

inline int QXmlStreamReaderPrivate::fastScanName(qint16 *prefix)
{
    int n = 0;
    uint c;
    while ((c = getChar()) != StreamEOF) {
        if (n >= 4096) {
            // This is too long to be a sensible name, and
            // can exhaust memory, or the range of decltype(*prefix)
            return 0;
        }
        switch (c) {
        case '\n':
        case ' ':
        case '\t':
        case '\r':
        case '&':
        case '#':
        case '\'':
        case '\"':
        case '<':
        case '>':
        case '[':
        case ']':
        case '=':
        case '%':
        case '/':
        case ';':
        case '?':
        case '!':
        case '^':
        case '|':
        case ',':
        case '(':
        case ')':
        case '+':
        case '*':
            putChar(c);
            if (prefix && *prefix == n+1) {
                *prefix = 0;
                putChar(':');
                --n;
            }
            return n;
        case ':':
            if (prefix) {
                if (*prefix == 0) {
                    *prefix = qint16(n + 2);
                } else { // only one colon allowed according to the namespace spec.
                    putChar(c);
                    return n;
                }
            } else {
                putChar(c);
                return n;
            }
            Q_FALLTHROUGH();
        default:
            textBuffer += QChar(ushort(c));
            ++n;
        }
    }

    if (prefix)
        *prefix = 0;
    qsizetype pos = textBuffer.size() - n;
    putString(textBuffer, pos);
    textBuffer.resize(pos);
    return 0;
}

enum NameChar { NameBeginning, NameNotBeginning, NotName };

static const char Begi = static_cast<char>(NameBeginning);
static const char NtBg = static_cast<char>(NameNotBeginning);
static const char NotN = static_cast<char>(NotName);

static const char nameCharTable[128] =
{
// 0x00
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
// 0x10
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
// 0x20 (0x2D is '-', 0x2E is '.')
    NotN, NotN, NotN, NotN, NotN, NotN, NotN, NotN,
    NotN, NotN, NotN, NotN, NotN, NtBg, NtBg, NotN,
// 0x30 (0x30..0x39 are '0'..'9', 0x3A is ':')
    NtBg, NtBg, NtBg, NtBg, NtBg, NtBg, NtBg, NtBg,
    NtBg, NtBg, Begi, NotN, NotN, NotN, NotN, NotN,
// 0x40 (0x41..0x5A are 'A'..'Z')
    NotN, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
// 0x50 (0x5F is '_')
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, NotN, NotN, NotN, NotN, Begi,
// 0x60 (0x61..0x7A are 'a'..'z')
    NotN, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
// 0x70
    Begi, Begi, Begi, Begi, Begi, Begi, Begi, Begi,
    Begi, Begi, Begi, NotN, NotN, NotN, NotN, NotN
};

static inline NameChar fastDetermineNameChar(QChar ch)
{
    ushort uc = ch.unicode();
    if (!(uc & ~0x7f)) // uc < 128
        return static_cast<NameChar>(nameCharTable[uc]);

    QChar::Category cat = ch.category();
    // ### some these categories might be slightly wrong
    if ((cat >= QChar::Letter_Uppercase && cat <= QChar::Letter_Other)
        || cat == QChar::Number_Letter)
        return NameBeginning;
    if ((cat >= QChar::Number_DecimalDigit && cat <= QChar::Number_Other)
                || (cat >= QChar::Mark_NonSpacing && cat <= QChar::Mark_Enclosing))
        return NameNotBeginning;
    return NotName;
}

inline int QXmlStreamReaderPrivate::fastScanNMTOKEN()
{
    int n = 0;
    uint c;
    while ((c = getChar()) != StreamEOF) {
        if (fastDetermineNameChar(QChar(c)) == NotName) {
            putChar(c);
            return n;
        } else {
            ++n;
            textBuffer += QChar(c);
        }
    }

    qsizetype pos = textBuffer.size() - n;
    putString(textBuffer, pos);
    textBuffer.resize(pos);

    return n;
}

void QXmlStreamReaderPrivate::putString(QStringView s, qsizetype from)
{
    if (from != 0) {
        putString(s.mid(from));
        return;
    }
    putStack.reserve(s.size());
    for (auto it = s.rbegin(), end = s.rend(); it != end; ++it)
        putStack.rawPush() = it->unicode();
}

void QXmlStreamReaderPrivate::putStringLiteral(QStringView s)
{
    putStack.reserve(s.size());
    for (auto it = s.rbegin(), end = s.rend(); it != end; ++it)
        putStack.rawPush() = ((LETTER << 16) | it->unicode());
}

void QXmlStreamReaderPrivate::putReplacement(QStringView s)
{
    putStack.reserve(s.size());
    for (auto it = s.rbegin(), end = s.rend(); it != end; ++it) {
        char16_t c = it->unicode();
        if (c == '\n' || c == '\r')
            putStack.rawPush() = ((LETTER << 16) | c);
        else
            putStack.rawPush() = c;
    }
}
void QXmlStreamReaderPrivate::putReplacementInAttributeValue(QStringView s)
{
    putStack.reserve(s.size());
    for (auto it = s.rbegin(), end = s.rend(); it != end; ++it) {
        char16_t c = it->unicode();
        if (c == '&' || c == ';')
            putStack.rawPush() = c;
        else if (c == '\n' || c == '\r')
            putStack.rawPush() = ' ';
        else
            putStack.rawPush() = ((LETTER << 16) | c);
    }
}

uint QXmlStreamReaderPrivate::getChar_helper()
{
    constexpr qsizetype BUFFER_SIZE = 8192;
    characterOffset += readBufferPos;
    readBufferPos = 0;
    if (readBuffer.size())
        readBuffer.resize(0);
    if (decoder.isValid())
        nbytesread = 0;
    if (device) {
        rawReadBuffer.resize(BUFFER_SIZE);
        qint64 nbytesreadOrMinus1 = device->read(rawReadBuffer.data() + nbytesread, BUFFER_SIZE - nbytesread);
        nbytesread += qMax(nbytesreadOrMinus1, qint64{0});
    } else {
        if (nbytesread)
            rawReadBuffer += dataBuffer;
        else
            rawReadBuffer = dataBuffer;
        nbytesread = rawReadBuffer.size();
        dataBuffer.clear();
    }
    if (!nbytesread) {
        atEnd = true;
        return StreamEOF;
    }

    if (!decoder.isValid()) {
        if (nbytesread < 4) { // the 4 is to cover 0xef 0xbb 0xbf plus
                              // one extra for the utf8 codec
            atEnd = true;
            return StreamEOF;
        }
        auto encoding = QStringDecoder::encodingForData(rawReadBuffer, char16_t('<'));
        if (!encoding)
            // assume utf-8
            encoding = QStringDecoder::Utf8;
        decoder = QStringDecoder(*encoding);
    }

    readBuffer = decoder(QByteArrayView(rawReadBuffer).first(nbytesread));

    if (lockEncoding && decoder.hasError()) {
        raiseWellFormedError(QXmlStream::tr("Encountered incorrectly encoded content."));
        readBuffer.clear();
        return StreamEOF;
    }

    readBuffer.reserve(1); // keep capacity when calling resize() next time

    if (readBufferPos < readBuffer.size()) {
        ushort c = readBuffer.at(readBufferPos++).unicode();
        return c;
    }

    atEnd = true;
    return StreamEOF;
}

XmlStringRef QXmlStreamReaderPrivate::namespaceForPrefix(QStringView prefix)
{
     for (const NamespaceDeclaration &namespaceDeclaration : reversed(namespaceDeclarations)) {
         if (namespaceDeclaration.prefix == prefix) {
             return namespaceDeclaration.namespaceUri;
         }
     }

#if 1
     if (namespaceProcessing && !prefix.isEmpty())
         raiseWellFormedError(QXmlStream::tr("Namespace prefix '%1' not declared").arg(prefix));
#endif

     return XmlStringRef();
}

/*
  uses namespaceForPrefix and builds the attribute vector
 */
void QXmlStreamReaderPrivate::resolveTag()
{
    const auto attributeStackCleaner = qScopeGuard([this](){ attributeStack.clear(); });
    const qsizetype n = attributeStack.size();

    if (namespaceProcessing) {
        for (DtdAttribute &dtdAttribute : dtdAttributes) {
            if (!dtdAttribute.isNamespaceAttribute
                || dtdAttribute.defaultValue.isNull()
                || dtdAttribute.tagName != qualifiedName
                || dtdAttribute.attributeQualifiedName.isNull())
                continue;
            qsizetype i = 0;
            while (i < n && symName(attributeStack[i].key) != dtdAttribute.attributeQualifiedName)
                ++i;
            if (i != n)
                continue;
            if (dtdAttribute.attributePrefix.isEmpty() && dtdAttribute.attributeName == QLatin1String("xmlns")) {
                NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                namespaceDeclaration.prefix.clear();

                const XmlStringRef ns(dtdAttribute.defaultValue);
                if (ns == QLatin1String("http://www.w3.org/2000/xmlns/") ||
                   ns == QLatin1String("http://www.w3.org/XML/1998/namespace"))
                    raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));
                else
                    namespaceDeclaration.namespaceUri = ns;
            } else if (dtdAttribute.attributePrefix == QLatin1String("xmlns")) {
                NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                XmlStringRef namespacePrefix = dtdAttribute.attributeName;
                XmlStringRef namespaceUri = dtdAttribute.defaultValue;
                if (((namespacePrefix == QLatin1String("xml"))
                     ^ (namespaceUri == QLatin1String("http://www.w3.org/XML/1998/namespace")))
                    || namespaceUri == QLatin1String("http://www.w3.org/2000/xmlns/")
                    || namespaceUri.isEmpty()
                    || namespacePrefix == QLatin1String("xmlns"))
                    raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));

                namespaceDeclaration.prefix = namespacePrefix;
                namespaceDeclaration.namespaceUri = namespaceUri;
            }
        }
    }

    tagStack.top().namespaceDeclaration.namespaceUri = namespaceUri = namespaceForPrefix(prefix);

    attributes.resize(n);

    for (qsizetype i = 0; i < n; ++i) {
        QXmlStreamAttribute &attribute = attributes[i];
        Attribute &attrib = attributeStack[i];
        XmlStringRef prefix(symPrefix(attrib.key));
        XmlStringRef name(symString(attrib.key));
        XmlStringRef qualifiedName(symName(attrib.key));
        XmlStringRef value(symString(attrib.value));

        attribute.m_name = name;
        attribute.m_qualifiedName = qualifiedName;
        attribute.m_value = value;

        if (!prefix.isEmpty()) {
            XmlStringRef attributeNamespaceUri = namespaceForPrefix(prefix);
            attribute.m_namespaceUri = XmlStringRef(attributeNamespaceUri);
        }

        for (qsizetype j = 0; j < i; ++j) {
            if (attributes[j].name() == attribute.name()
                && attributes[j].namespaceUri() == attribute.namespaceUri()
                && (namespaceProcessing || attributes[j].qualifiedName() == attribute.qualifiedName()))
            {
                raiseWellFormedError(QXmlStream::tr("Attribute '%1' redefined.").arg(attribute.qualifiedName()));
                return;
            }
        }
    }

    for (DtdAttribute &dtdAttribute : dtdAttributes) {
        if (dtdAttribute.isNamespaceAttribute
            || dtdAttribute.defaultValue.isNull()
            || dtdAttribute.tagName != qualifiedName
            || dtdAttribute.attributeQualifiedName.isNull())
            continue;
        qsizetype i = 0;
        while (i < n && symName(attributeStack[i].key) != dtdAttribute.attributeQualifiedName)
            ++i;
        if (i != n)
            continue;



        QXmlStreamAttribute attribute;
        attribute.m_name = dtdAttribute.attributeName;
        attribute.m_qualifiedName = dtdAttribute.attributeQualifiedName;
        attribute.m_value = dtdAttribute.defaultValue;

        if (!dtdAttribute.attributePrefix.isEmpty()) {
            XmlStringRef attributeNamespaceUri = namespaceForPrefix(dtdAttribute.attributePrefix);
            attribute.m_namespaceUri = XmlStringRef(attributeNamespaceUri);
        }
        attribute.m_isDefault = true;
        attributes.append(std::move(attribute));
    }
}

void QXmlStreamReaderPrivate::resolvePublicNamespaces()
{
    const Tag &tag = tagStack.top();
    qsizetype n = namespaceDeclarations.size() - tag.namespaceDeclarationsSize;
    publicNamespaceDeclarations.resize(n);
    for (qsizetype i = 0; i < n; ++i) {
        const NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.at(tag.namespaceDeclarationsSize + i);
        QXmlStreamNamespaceDeclaration &publicNamespaceDeclaration = publicNamespaceDeclarations[i];
        publicNamespaceDeclaration.m_prefix = namespaceDeclaration.prefix;
        publicNamespaceDeclaration.m_namespaceUri = namespaceDeclaration.namespaceUri;
    }
}

void QXmlStreamReaderPrivate::resolveDtd()
{
    publicNotationDeclarations.resize(notationDeclarations.size());
    for (qsizetype i = 0; i < notationDeclarations.size(); ++i) {
        const QXmlStreamReaderPrivate::NotationDeclaration &notationDeclaration = notationDeclarations.at(i);
        QXmlStreamNotationDeclaration &publicNotationDeclaration = publicNotationDeclarations[i];
        publicNotationDeclaration.m_name = notationDeclaration.name;
        publicNotationDeclaration.m_systemId = notationDeclaration.systemId;
        publicNotationDeclaration.m_publicId = notationDeclaration.publicId;

    }
    notationDeclarations.clear();
    publicEntityDeclarations.resize(entityDeclarations.size());
    for (qsizetype i = 0; i < entityDeclarations.size(); ++i) {
        const QXmlStreamReaderPrivate::EntityDeclaration &entityDeclaration = entityDeclarations.at(i);
        QXmlStreamEntityDeclaration &publicEntityDeclaration = publicEntityDeclarations[i];
        publicEntityDeclaration.m_name = entityDeclaration.name;
        publicEntityDeclaration.m_notationName = entityDeclaration.notationName;
        publicEntityDeclaration.m_systemId = entityDeclaration.systemId;
        publicEntityDeclaration.m_publicId = entityDeclaration.publicId;
        publicEntityDeclaration.m_value = entityDeclaration.value;
    }
    entityDeclarations.clear();
    parameterEntityHash.clear();
}

uint QXmlStreamReaderPrivate::resolveCharRef(int symbolIndex)
{
    bool ok = true;
    uint s;
    // ### add toXShort to XmlString?
    if (sym(symbolIndex).c == 'x')
        s = symString(symbolIndex, 1).view().toUInt(&ok, 16);
    else
        s = symString(symbolIndex).view().toUInt(&ok, 10);

    ok &= (s == 0x9 || s == 0xa || s == 0xd || (s >= 0x20 && s <= 0xd7ff)
           || (s >= 0xe000 && s <= 0xfffd) || (s >= 0x10000 && s <= QChar::LastValidCodePoint));

    return ok ? s : 0;
}


void QXmlStreamReaderPrivate::checkPublicLiteral(QStringView publicId)
{
//#x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%]

    const ushort *data = reinterpret_cast<const ushort *>(publicId.constData());
    uchar c = 0;
    qsizetype i;
    for (i = publicId.size() - 1; i >= 0; --i) {
        if (data[i] < 256)
            switch ((c = data[i])) {
            case ' ': case '\n': case '\r': case '-': case '(': case ')':
            case '+': case ',': case '.': case '/': case ':': case '=':
            case '?': case ';': case '!': case '*': case '#': case '@':
            case '$': case '_': case '%': case '\'': case '\"':
                continue;
            default:
                if ((c >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z')
                    || (c >= '0' && c <= '9'))
                    continue;
            }
        break;
    }
    if (i >= 0)
        raiseWellFormedError(QXmlStream::tr("Unexpected character '%1' in public id literal.").arg(QChar(QLatin1Char(c))));
}

/*
  Checks whether the document starts with an xml declaration. If it
  does, this function returns \c true; otherwise it sets up everything
  for a synthetic start document event and returns \c false.
 */
bool QXmlStreamReaderPrivate::checkStartDocument()
{
    hasCheckedStartDocument = true;

    if (scanString(spell[XML], XML))
        return true;

    type = QXmlStreamReader::StartDocument;
    if (atEnd) {
        hasCheckedStartDocument = false;
        raiseError(QXmlStreamReader::PrematureEndOfDocumentError);
    }
    return false;
}

void QXmlStreamReaderPrivate::startDocument()
{
    QString err;
    if (documentVersion != QLatin1String("1.0")) {
        if (documentVersion.view().contains(QLatin1Char(' ')))
            err = QXmlStream::tr("Invalid XML version string.");
        else
            err = QXmlStream::tr("Unsupported XML version.");
    }
    qsizetype n = attributeStack.size();

    /* We use this bool to ensure that the pesudo attributes are in the
     * proper order:
     *
     * [23]     XMLDecl     ::=     '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>' */
    bool hasStandalone = false;

    for (qsizetype i = 0; err.isNull() && i < n; ++i) {
        Attribute &attrib = attributeStack[i];
        XmlStringRef prefix(symPrefix(attrib.key));
        XmlStringRef key(symString(attrib.key));
        XmlStringRef value(symString(attrib.value));

        if (prefix.isEmpty() && key == QLatin1String("encoding")) {
            documentEncoding = value;

            if (hasStandalone)
                err = QXmlStream::tr("The standalone pseudo attribute must appear after the encoding.");
            if (!QXmlUtils::isEncName(value))
                err = QXmlStream::tr("%1 is an invalid encoding name.").arg(value);
            else {
                QByteArray enc = value.toString().toUtf8();
                if (!lockEncoding) {
                    decoder = QStringDecoder(enc.constData());
                    if (!decoder.isValid()) {
                        err = QXmlStream::tr("Encoding %1 is unsupported").arg(value);
                    } else {
                        readBuffer = decoder(QByteArrayView(rawReadBuffer).first(nbytesread));
                    }
                }
            }
        } else if (prefix.isEmpty() && key == QLatin1String("standalone")) {
            hasStandalone = true;
            if (value == QLatin1String("yes"))
                standalone = true;
            else if (value == QLatin1String("no"))
                standalone = false;
            else
                err = QXmlStream::tr("Standalone accepts only yes or no.");
        } else {
            err = QXmlStream::tr("Invalid attribute in XML declaration.");
        }
    }

    if (!err.isNull())
        raiseWellFormedError(err);
    attributeStack.clear();
}


void QXmlStreamReaderPrivate::raiseError(QXmlStreamReader::Error error, const QString& message)
{
    this->error = error;
    errorString = message;
    if (errorString.isNull()) {
        if (error == QXmlStreamReader::PrematureEndOfDocumentError)
            errorString = QXmlStream::tr("Premature end of document.");
        else if (error == QXmlStreamReader::CustomError)
            errorString = QXmlStream::tr("Invalid document.");
    }

    type = QXmlStreamReader::Invalid;
}

void QXmlStreamReaderPrivate::raiseWellFormedError(const QString &message)
{
    raiseError(QXmlStreamReader::NotWellFormedError, message);
}

void QXmlStreamReaderPrivate::parseError()
{

    if (token == EOF_SYMBOL) {
        raiseError(QXmlStreamReader::PrematureEndOfDocumentError);
        return;
    }
    const int nmax = 4;
    QString error_message;
    int ers = state_stack[tos];
    int nexpected = 0;
    int expected[nmax];
    if (token != ERROR)
        for (int tk = 0; tk < TERMINAL_COUNT; ++tk) {
            int k = t_action(ers, tk);
            if (k <= 0)
                continue;
            if (spell[tk]) {
                if (nexpected < nmax)
                    expected[nexpected++] = tk;
            }
        }

    if (nexpected && nexpected < nmax) {
        //: '<first option>'
        QString exp_str = QXmlStream::tr("'%1'", "expected").arg(QLatin1String(spell[expected[0]]));
        if (nexpected == 2) {
            //: <first option>, '<second option>'
            exp_str = QXmlStream::tr("%1 or '%2'", "expected").arg(exp_str, QLatin1String(spell[expected[1]]));
        } else if (nexpected > 2) {
            int s = 1;
            for (; s < nexpected - 1; ++s) {
                //: <options so far>, '<next option>'
                exp_str = QXmlStream::tr("%1, '%2'", "expected").arg(exp_str, QLatin1String(spell[expected[s]]));
            }
            //: <options so far>, or '<final option>'
            exp_str = QXmlStream::tr("%1, or '%2'", "expected").arg(exp_str, QLatin1String(spell[expected[s]]));
        }
        error_message = QXmlStream::tr("Expected %1, but got '%2'.").arg(exp_str, QLatin1String(spell[token]));
    } else {
        error_message = QXmlStream::tr("Unexpected '%1'.").arg(QLatin1String(spell[token]));
    }

    raiseWellFormedError(error_message);
}

void QXmlStreamReaderPrivate::resume(int rule) {
    resumeReduction = rule;
    if (error == QXmlStreamReader::NoError)
        raiseError(QXmlStreamReader::PrematureEndOfDocumentError);
}

/*! Returns the current line number, starting with 1.

\sa columnNumber(), characterOffset()
 */
qint64 QXmlStreamReader::lineNumber() const
{
    Q_D(const QXmlStreamReader);
    return d->lineNumber + 1; // in public we start with 1
}

/*! Returns the current column number, starting with 0.

\sa lineNumber(), characterOffset()
 */
qint64 QXmlStreamReader::columnNumber() const
{
    Q_D(const QXmlStreamReader);
    return d->characterOffset - d->lastLineStart + d->readBufferPos;
}

/*! Returns the current character offset, starting with 0.

\sa lineNumber(), columnNumber()
*/
qint64 QXmlStreamReader::characterOffset() const
{
    Q_D(const QXmlStreamReader);
    return d->characterOffset + d->readBufferPos;
}


/*!  Returns the text of \l Characters, \l Comment, \l DTD, or
  EntityReference.
 */
QStringView QXmlStreamReader::text() const
{
    Q_D(const QXmlStreamReader);
    return d->text;
}


/*!  If the tokenType() is \l DTD, this function returns the DTD's
  notation declarations. Otherwise an empty vector is returned.

  The QXmlStreamNotationDeclarations class is defined to be a QList
  of QXmlStreamNotationDeclaration.
 */
QXmlStreamNotationDeclarations QXmlStreamReader::notationDeclarations() const
{
    Q_D(const QXmlStreamReader);
    if (d->notationDeclarations.size())
        const_cast<QXmlStreamReaderPrivate *>(d)->resolveDtd();
    return d->publicNotationDeclarations;
}


/*!  If the tokenType() is \l DTD, this function returns the DTD's
  unparsed (external) entity declarations. Otherwise an empty vector is returned.

  The QXmlStreamEntityDeclarations class is defined to be a QList
  of QXmlStreamEntityDeclaration.
 */
QXmlStreamEntityDeclarations QXmlStreamReader::entityDeclarations() const
{
    Q_D(const QXmlStreamReader);
    if (d->entityDeclarations.size())
        const_cast<QXmlStreamReaderPrivate *>(d)->resolveDtd();
    return d->publicEntityDeclarations;
}

/*!
  \since 4.4

  If the tokenType() is \l DTD, this function returns the DTD's
  name. Otherwise an empty string is returned.

 */
QStringView QXmlStreamReader::dtdName() const
{
   Q_D(const QXmlStreamReader);
   if (d->type == QXmlStreamReader::DTD)
       return d->dtdName;
   return QStringView();
}

/*!
  \since 4.4

  If the tokenType() is \l DTD, this function returns the DTD's
  public identifier. Otherwise an empty string is returned.

 */
QStringView QXmlStreamReader::dtdPublicId() const
{
   Q_D(const QXmlStreamReader);
   if (d->type == QXmlStreamReader::DTD)
       return d->dtdPublicId;
   return QStringView();
}

/*!
  \since 4.4

  If the tokenType() is \l DTD, this function returns the DTD's
  system identifier. Otherwise an empty string is returned.

 */
QStringView QXmlStreamReader::dtdSystemId() const
{
   Q_D(const QXmlStreamReader);
   if (d->type == QXmlStreamReader::DTD)
       return d->dtdSystemId;
   return QStringView();
}

/*!
  \since 5.15

  Returns the maximum amount of characters a single entity is
  allowed to expand into. If a single entity expands past the
  given limit, the document is not considered well formed.

  \sa setEntityExpansionLimit
*/
int QXmlStreamReader::entityExpansionLimit() const
{
    Q_D(const QXmlStreamReader);
    return d->entityExpansionLimit;
}

/*!
  \since 5.15

  Sets the maximum amount of characters a single entity is
  allowed to expand into to \a limit. If a single entity expands
  past the given limit, the document is not considered well formed.

  The limit is there to prevent DoS attacks when loading unknown
  XML documents where recursive entity expansion could otherwise
  exhaust all available memory.

  The default value for this property is 4096 characters.

  \sa entityExpansionLimit
*/
void QXmlStreamReader::setEntityExpansionLimit(int limit)
{
    Q_D(QXmlStreamReader);
    d->entityExpansionLimit = limit;
}

/*!  If the tokenType() is \l StartElement, this function returns the
  element's namespace declarations. Otherwise an empty vector is
  returned.

  The QXmlStreamNamespaceDeclarations class is defined to be a QList
  of QXmlStreamNamespaceDeclaration.

  \sa addExtraNamespaceDeclaration(), addExtraNamespaceDeclarations()
 */
QXmlStreamNamespaceDeclarations QXmlStreamReader::namespaceDeclarations() const
{
    Q_D(const QXmlStreamReader);
    if (d->publicNamespaceDeclarations.isEmpty() && d->type == StartElement)
        const_cast<QXmlStreamReaderPrivate *>(d)->resolvePublicNamespaces();
    return d->publicNamespaceDeclarations;
}


/*!
  \since 4.4

  Adds an \a extraNamespaceDeclaration. The declaration will be
  valid for children of the current element, or - should the function
  be called before any elements are read - for the entire XML
  document.

  \sa namespaceDeclarations(), addExtraNamespaceDeclarations(), setNamespaceProcessing()
 */
void QXmlStreamReader::addExtraNamespaceDeclaration(const QXmlStreamNamespaceDeclaration &extraNamespaceDeclaration)
{
    Q_D(QXmlStreamReader);
    QXmlStreamReaderPrivate::NamespaceDeclaration &namespaceDeclaration = d->namespaceDeclarations.push();
    namespaceDeclaration.prefix = d->addToStringStorage(extraNamespaceDeclaration.prefix());
    namespaceDeclaration.namespaceUri = d->addToStringStorage(extraNamespaceDeclaration.namespaceUri());
}

/*!
  \since 4.4

  Adds a vector of declarations specified by \a extraNamespaceDeclarations.

  \sa namespaceDeclarations(), addExtraNamespaceDeclaration()
 */
void QXmlStreamReader::addExtraNamespaceDeclarations(const QXmlStreamNamespaceDeclarations &extraNamespaceDeclarations)
{
    for (const auto &extraNamespaceDeclaration : extraNamespaceDeclarations)
        addExtraNamespaceDeclaration(extraNamespaceDeclaration);
}


/*!  Convenience function to be called in case a StartElement was
  read. Reads until the corresponding EndElement and returns all text
  in-between. In case of no error, the current token (see tokenType())
  after having called this function is EndElement.

  The function concatenates text() when it reads either \l Characters
  or EntityReference tokens, but skips ProcessingInstruction and \l
  Comment. If the current token is not StartElement, an empty string is
  returned.

  The \a behaviour defines what happens in case anything else is
  read before reaching EndElement. The function can include the text from
  child elements (useful for example for HTML), ignore child elements, or
  raise an UnexpectedElementError and return what was read so far (default).

  \since 4.6
 */
QString QXmlStreamReader::readElementText(ReadElementTextBehaviour behaviour)
{
    Q_D(QXmlStreamReader);
    if (isStartElement()) {
        QString result;
        forever {
            switch (readNext()) {
            case Characters:
            case EntityReference:
                result.insert(result.size(), d->text);
                break;
            case EndElement:
                return result;
            case ProcessingInstruction:
            case Comment:
                break;
            case StartElement:
                if (behaviour == SkipChildElements) {
                    skipCurrentElement();
                    break;
                } else if (behaviour == IncludeChildElements) {
                    result += readElementText(behaviour);
                    break;
                }
                Q_FALLTHROUGH();
            default:
                if (d->error || behaviour == ErrorOnUnexpectedElement) {
                    if (!d->error)
                        d->raiseError(UnexpectedElementError, QXmlStream::tr("Expected character data."));
                    return result;
                }
            }
        }
    }
    return QString();
}

/*!  Raises a custom error with an optional error \a message.

  \sa error(), errorString()
 */
void QXmlStreamReader::raiseError(const QString& message)
{
    Q_D(QXmlStreamReader);
    d->raiseError(CustomError, message);
}

/*!
  Returns the error message that was set with raiseError().

  \sa error(), lineNumber(), columnNumber(), characterOffset()
 */
QString QXmlStreamReader::errorString() const
{
    Q_D(const QXmlStreamReader);
    if (d->type == QXmlStreamReader::Invalid)
        return d->errorString;
    return QString();
}

/*!  Returns the type of the current error, or NoError if no error occurred.

  \sa errorString(), raiseError()
 */
QXmlStreamReader::Error QXmlStreamReader::error() const
{
    Q_D(const QXmlStreamReader);
    if (d->type == QXmlStreamReader::Invalid)
        return d->error;
    return NoError;
}

/*!
  Returns the target of a ProcessingInstruction.
 */
QStringView QXmlStreamReader::processingInstructionTarget() const
{
    Q_D(const QXmlStreamReader);
    return d->processingInstructionTarget;
}

/*!
  Returns the data of a ProcessingInstruction.
 */
QStringView QXmlStreamReader::processingInstructionData() const
{
    Q_D(const QXmlStreamReader);
    return d->processingInstructionData;
}



/*!
  Returns the local name of a StartElement, EndElement, or an EntityReference.

  \sa namespaceUri(), qualifiedName()
 */
QStringView QXmlStreamReader::name() const
{
    Q_D(const QXmlStreamReader);
    return d->name;
}

/*!
  Returns the namespaceUri of a StartElement or EndElement.

  \sa name(), qualifiedName()
 */
QStringView QXmlStreamReader::namespaceUri() const
{
    Q_D(const QXmlStreamReader);
    return d->namespaceUri;
}

/*!
  Returns the qualified name of a StartElement or EndElement;

  A qualified name is the raw name of an element in the XML data. It
  consists of the namespace prefix, followed by colon, followed by the
  element's local name. Since the namespace prefix is not unique (the
  same prefix can point to different namespaces and different prefixes
  can point to the same namespace), you shouldn't use qualifiedName(),
  but the resolved namespaceUri() and the attribute's local name().

   \sa name(), prefix(), namespaceUri()
 */
QStringView QXmlStreamReader::qualifiedName() const
{
    Q_D(const QXmlStreamReader);
    return d->qualifiedName;
}



/*!
  \since 4.4

  Returns the prefix of a StartElement or EndElement.

  \sa name(), qualifiedName()
*/
QStringView QXmlStreamReader::prefix() const
{
    Q_D(const QXmlStreamReader);
    return d->prefix;
}

/*!
  Returns the attributes of a StartElement.
 */
QXmlStreamAttributes QXmlStreamReader::attributes() const
{
    Q_D(const QXmlStreamReader);
    return d->attributes;
}

#endif // QT_NO_XMLSTREAMREADER

/*!
    \class QXmlStreamAttribute
    \inmodule QtCore
    \since 4.3
    \reentrant
    \brief The QXmlStreamAttribute class represents a single XML attribute.

    \ingroup xml-tools

    An attribute consists of an optionally empty namespaceUri(), a
    name(), a value(), and an isDefault() attribute.

    The raw XML attribute name is returned as qualifiedName().
*/

/*!
  Creates an empty attribute.
 */
QXmlStreamAttribute::QXmlStreamAttribute()
{
    m_isDefault = false;
}

/*!  Constructs an attribute in the namespace described with \a
  namespaceUri with \a name and value \a value.
 */
QXmlStreamAttribute::QXmlStreamAttribute(const QString &namespaceUri, const QString &name, const QString &value)
{
    m_namespaceUri = namespaceUri;
    m_name = m_qualifiedName = name;
    m_value = value;
    m_namespaceUri = namespaceUri;
}

/*!
    Constructs an attribute with qualified name \a qualifiedName and value \a value.
 */
QXmlStreamAttribute::QXmlStreamAttribute(const QString &qualifiedName, const QString &value)
{
    qsizetype colon = qualifiedName.indexOf(u':');
    m_name = qualifiedName.mid(colon + 1);
    m_qualifiedName = qualifiedName;
    m_value = value;
}

/*! \fn QStringView QXmlStreamAttribute::namespaceUri() const

   Returns the attribute's resolved namespaceUri, or an empty string
   reference if the attribute does not have a defined namespace.
 */
/*! \fn QStringView QXmlStreamAttribute::name() const
   Returns the attribute's local name.
 */
/*! \fn QStringView QXmlStreamAttribute::qualifiedName() const
   Returns the attribute's qualified name.

   A qualified name is the raw name of an attribute in the XML
   data. It consists of the namespace prefix(), followed by colon,
   followed by the attribute's local name(). Since the namespace prefix
   is not unique (the same prefix can point to different namespaces
   and different prefixes can point to the same namespace), you
   shouldn't use qualifiedName(), but the resolved namespaceUri() and
   the attribute's local name().
 */
/*!
   \fn QStringView QXmlStreamAttribute::prefix() const
   \since 4.4
   Returns the attribute's namespace prefix.

   \sa name(), qualifiedName()

*/

/*! \fn QStringView QXmlStreamAttribute::value() const
   Returns the attribute's value.
 */

/*! \fn bool QXmlStreamAttribute::isDefault() const

   Returns \c true if the parser added this attribute with a default
   value following an ATTLIST declaration in the DTD; otherwise
   returns \c false.
*/
/*! \fn bool QXmlStreamAttribute::operator==(const QXmlStreamAttribute &other) const

    Compares this attribute with \a other and returns \c true if they are
    equal; otherwise returns \c false.
 */
/*! \fn bool QXmlStreamAttribute::operator!=(const QXmlStreamAttribute &other) const

    Compares this attribute with \a other and returns \c true if they are
    not equal; otherwise returns \c false.
 */

/*!
    \class QXmlStreamAttributes
    \inmodule QtCore
    \since 4.3
    \reentrant
    \brief The QXmlStreamAttributes class represents a vector of QXmlStreamAttribute.

    Attributes are returned by a QXmlStreamReader in
    \l{QXmlStreamReader::attributes()} {attributes()} when the reader
    reports a \l {QXmlStreamReader::StartElement}{start element}. The
    class can also be used with a QXmlStreamWriter as an argument to
    \l {QXmlStreamWriter::writeAttributes()}{writeAttributes()}.

    The convenience function value() loops over the vector and returns
    an attribute value for a given namespaceUri and an attribute's
    name.

    New attributes can be added with append().

    \ingroup xml-tools
*/

/*!
    \fn QXmlStreamAttributes::QXmlStreamAttributes()

    A constructor for QXmlStreamAttributes.
*/

/*!
    \typedef QXmlStreamNotationDeclarations
    \relates QXmlStreamNotationDeclaration

    Synonym for QList<QXmlStreamNotationDeclaration>.
*/


/*!
    \class QXmlStreamNotationDeclaration
    \inmodule QtCore
    \since 4.3
    \reentrant
    \brief The QXmlStreamNotationDeclaration class represents a DTD notation declaration.

    \ingroup xml-tools

    An notation declaration consists of a name(), a systemId(), and a publicId().
*/

/*!
  Creates an empty notation declaration.
*/
QXmlStreamNotationDeclaration::QXmlStreamNotationDeclaration()
{
}

/*! \fn QStringView QXmlStreamNotationDeclaration::name() const

Returns the notation name.
*/
/*! \fn QStringView QXmlStreamNotationDeclaration::systemId() const

Returns the system identifier.
*/
/*! \fn QStringView QXmlStreamNotationDeclaration::publicId() const

Returns the public identifier.
*/

/*! \fn inline bool QXmlStreamNotationDeclaration::operator==(const QXmlStreamNotationDeclaration &other) const

    Compares this notation declaration with \a other and returns \c true
    if they are equal; otherwise returns \c false.
 */
/*! \fn inline bool QXmlStreamNotationDeclaration::operator!=(const QXmlStreamNotationDeclaration &other) const

    Compares this notation declaration with \a other and returns \c true
    if they are not equal; otherwise returns \c false.
 */

/*!
    \typedef QXmlStreamNamespaceDeclarations
    \relates QXmlStreamNamespaceDeclaration

    Synonym for QList<QXmlStreamNamespaceDeclaration>.
*/

/*!
    \class QXmlStreamNamespaceDeclaration
    \inmodule QtCore
    \since 4.3
    \reentrant
    \brief The QXmlStreamNamespaceDeclaration class represents a namespace declaration.

    \ingroup xml-tools

    An namespace declaration consists of a prefix() and a namespaceUri().
*/
/*! \fn inline bool QXmlStreamNamespaceDeclaration::operator==(const QXmlStreamNamespaceDeclaration &other) const

    Compares this namespace declaration with \a other and returns \c true
    if they are equal; otherwise returns \c false.
 */
/*! \fn inline bool QXmlStreamNamespaceDeclaration::operator!=(const QXmlStreamNamespaceDeclaration &other) const

    Compares this namespace declaration with \a other and returns \c true
    if they are not equal; otherwise returns \c false.
 */

/*!
  Creates an empty namespace declaration.
*/
QXmlStreamNamespaceDeclaration::QXmlStreamNamespaceDeclaration()
{
}

/*!
  \since 4.4

  Creates a namespace declaration with \a prefix and \a namespaceUri.
*/
QXmlStreamNamespaceDeclaration::QXmlStreamNamespaceDeclaration(const QString &prefix, const QString &namespaceUri)
{
    m_prefix = prefix;
    m_namespaceUri = namespaceUri;
}

/*! \fn QStringView QXmlStreamNamespaceDeclaration::prefix() const

Returns the prefix.
*/
/*! \fn QStringView QXmlStreamNamespaceDeclaration::namespaceUri() const

Returns the namespaceUri.
*/




/*!
    \typedef QXmlStreamEntityDeclarations
    \relates QXmlStreamEntityDeclaration

    Synonym for QList<QXmlStreamEntityDeclaration>.
*/

/*!
    \class QXmlString
    \inmodule QtCore
    \since 6.0
    \internal
*/

/*!
    \class QXmlStreamEntityDeclaration
    \inmodule QtCore
    \since 4.3
    \reentrant
    \brief The QXmlStreamEntityDeclaration class represents a DTD entity declaration.

    \ingroup xml-tools

    An entity declaration consists of a name(), a notationName(), a
    systemId(), a publicId(), and a value().
*/

/*!
  Creates an empty entity declaration.
*/
QXmlStreamEntityDeclaration::QXmlStreamEntityDeclaration()
{
}

/*! \fn QStringView QXmlStreamEntityDeclaration::name() const

Returns the entity name.
*/
/*! \fn QStringView QXmlStreamEntityDeclaration::notationName() const

Returns the notation name.
*/
/*! \fn QStringView QXmlStreamEntityDeclaration::systemId() const

Returns the system identifier.
*/
/*! \fn QStringView QXmlStreamEntityDeclaration::publicId() const

Returns the public identifier.
*/
/*! \fn QStringView QXmlStreamEntityDeclaration::value() const

Returns the entity's value.
*/

/*! \fn bool QXmlStreamEntityDeclaration::operator==(const QXmlStreamEntityDeclaration &other) const

    Compares this entity declaration with \a other and returns \c true if
    they are equal; otherwise returns \c false.
 */
/*! \fn bool QXmlStreamEntityDeclaration::operator!=(const QXmlStreamEntityDeclaration &other) const

    Compares this entity declaration with \a other and returns \c true if
    they are not equal; otherwise returns \c false.
 */

/*!  Returns the value of the attribute \a name in the namespace
  described with \a namespaceUri, or an empty string reference if the
  attribute is not defined. The \a namespaceUri can be empty.
 */
QStringView QXmlStreamAttributes::value(const QString &namespaceUri, const QString &name) const
{
    for (const QXmlStreamAttribute &attribute : *this) {
        if (attribute.name() == name && attribute.namespaceUri() == namespaceUri)
            return attribute.value();
    }
    return QStringView();
}

/*!\overload
  Returns the value of the attribute \a name in the namespace
  described with \a namespaceUri, or an empty string reference if the
  attribute is not defined. The \a namespaceUri can be empty.
 */
QStringView QXmlStreamAttributes::value(const QString &namespaceUri, QLatin1String name) const
{
    for (const QXmlStreamAttribute &attribute : *this) {
        if (attribute.name() == name && attribute.namespaceUri() == namespaceUri)
            return attribute.value();
    }
    return QStringView();
}

/*!\overload
  Returns the value of the attribute \a name in the namespace
  described with \a namespaceUri, or an empty string reference if the
  attribute is not defined. The \a namespaceUri can be empty.
 */
QStringView QXmlStreamAttributes::value(QLatin1String namespaceUri, QLatin1String name) const
{
    for (const QXmlStreamAttribute &attribute : *this) {
        if (attribute.name() == name && attribute.namespaceUri() == namespaceUri)
            return attribute.value();
    }
    return QStringView();
}

/*!\overload

  Returns the value of the attribute with qualified name \a
  qualifiedName , or an empty string reference if the attribute is not
  defined. A qualified name is the raw name of an attribute in the XML
  data. It consists of the namespace prefix, followed by colon,
  followed by the attribute's local name. Since the namespace prefix
  is not unique (the same prefix can point to different namespaces and
  different prefixes can point to the same namespace), you shouldn't
  use qualified names, but a resolved namespaceUri and the attribute's
  local name.
 */
QStringView QXmlStreamAttributes::value(const QString &qualifiedName) const
{
    for (const QXmlStreamAttribute &attribute : *this) {
        if (attribute.qualifiedName() == qualifiedName)
            return attribute.value();
    }
    return QStringView();
}

/*!\overload

  Returns the value of the attribute with qualified name \a
  qualifiedName , or an empty string reference if the attribute is not
  defined. A qualified name is the raw name of an attribute in the XML
  data. It consists of the namespace prefix, followed by colon,
  followed by the attribute's local name. Since the namespace prefix
  is not unique (the same prefix can point to different namespaces and
  different prefixes can point to the same namespace), you shouldn't
  use qualified names, but a resolved namespaceUri and the attribute's
  local name.
 */
QStringView QXmlStreamAttributes::value(QLatin1String qualifiedName) const
{
    for (const QXmlStreamAttribute &attribute : *this) {
        if (attribute.qualifiedName() == qualifiedName)
            return attribute.value();
    }
    return QStringView();
}

/*!Appends a new attribute with \a name in the namespace
  described with \a namespaceUri, and value \a value. The \a
  namespaceUri can be empty.
 */
void QXmlStreamAttributes::append(const QString &namespaceUri, const QString &name, const QString &value)
{
    append(QXmlStreamAttribute(namespaceUri, name, value));
}

/*!\overload
  Appends a new attribute with qualified name \a qualifiedName and
  value \a value.
 */
void QXmlStreamAttributes::append(const QString &qualifiedName, const QString &value)
{
    append(QXmlStreamAttribute(qualifiedName, value));
}

#ifndef QT_NO_XMLSTREAMREADER

/*! \fn bool QXmlStreamReader::isStartDocument() const
  Returns \c true if tokenType() equals \l StartDocument; otherwise returns \c false.
*/
/*! \fn bool QXmlStreamReader::isEndDocument() const
  Returns \c true if tokenType() equals \l EndDocument; otherwise returns \c false.
*/
/*! \fn bool QXmlStreamReader::isStartElement() const
  Returns \c true if tokenType() equals \l StartElement; otherwise returns \c false.
*/
/*! \fn bool QXmlStreamReader::isEndElement() const
  Returns \c true if tokenType() equals \l EndElement; otherwise returns \c false.
*/
/*! \fn bool QXmlStreamReader::isCharacters() const
  Returns \c true if tokenType() equals \l Characters; otherwise returns \c false.

  \sa isWhitespace(), isCDATA()
*/
/*! \fn bool QXmlStreamReader::isComment() const
  Returns \c true if tokenType() equals \l Comment; otherwise returns \c false.
*/
/*! \fn bool QXmlStreamReader::isDTD() const
  Returns \c true if tokenType() equals \l DTD; otherwise returns \c false.
*/
/*! \fn bool QXmlStreamReader::isEntityReference() const
  Returns \c true if tokenType() equals \l EntityReference; otherwise returns \c false.
*/
/*! \fn bool QXmlStreamReader::isProcessingInstruction() const
  Returns \c true if tokenType() equals \l ProcessingInstruction; otherwise returns \c false.
*/

/*!  Returns \c true if the reader reports characters that only consist
  of white-space; otherwise returns \c false.

  \sa isCharacters(), text()
*/
bool QXmlStreamReader::isWhitespace() const
{
    Q_D(const QXmlStreamReader);
    return d->type == QXmlStreamReader::Characters && d->isWhitespace;
}

/*!  Returns \c true if the reader reports characters that stem from a
  CDATA section; otherwise returns \c false.

  \sa isCharacters(), text()
*/
bool QXmlStreamReader::isCDATA() const
{
    Q_D(const QXmlStreamReader);
    return d->type == QXmlStreamReader::Characters && d->isCDATA;
}



/*!
  Returns \c true if this document has been declared standalone in the
  XML declaration; otherwise returns \c false.

  If no XML declaration has been parsed, this function returns \c false.
 */
bool QXmlStreamReader::isStandaloneDocument() const
{
    Q_D(const QXmlStreamReader);
    return d->standalone;
}


/*!
     \since 4.4

     If the tokenType() is \l StartDocument, this function returns the
     version string as specified in the XML declaration.
     Otherwise an empty string is returned.
 */
QStringView QXmlStreamReader::documentVersion() const
{
   Q_D(const QXmlStreamReader);
   if (d->type == QXmlStreamReader::StartDocument)
       return d->documentVersion;
   return QStringView();
}

/*!
     \since 4.4

     If the tokenType() is \l StartDocument, this function returns the
     encoding string as specified in the XML declaration.
     Otherwise an empty string is returned.
 */
QStringView QXmlStreamReader::documentEncoding() const
{
   Q_D(const QXmlStreamReader);
   if (d->type == QXmlStreamReader::StartDocument)
       return d->documentEncoding;
   return QStringView();
}

#endif // QT_NO_XMLSTREAMREADER

/*!
  \class QXmlStreamWriter
  \inmodule QtCore
  \since 4.3
  \reentrant

  \brief The QXmlStreamWriter class provides an XML writer with a
  simple streaming API.

  \ingroup xml-tools

  QXmlStreamWriter is the counterpart to QXmlStreamReader for writing
  XML. Like its related class, it operates on a QIODevice specified
  with setDevice(). The API is simple and straightforward: for every
  XML token or event you want to write, the writer provides a
  specialized function.

  You start a document with writeStartDocument() and end it with
  writeEndDocument(). This will implicitly close all remaining open
  tags.

  Element tags are opened with writeStartElement() followed by
  writeAttribute() or writeAttributes(), element content, and then
  writeEndElement(). A shorter form writeEmptyElement() can be used
  to write empty elements, followed by writeAttributes().

  Element content consists of either characters, entity references or
  nested elements. It is written with writeCharacters(), which also
  takes care of escaping all forbidden characters and character
  sequences, writeEntityReference(), or subsequent calls to
  writeStartElement(). A convenience method writeTextElement() can be
  used for writing terminal elements that contain nothing but text.

  The following abridged code snippet shows the basic use of the class
  to write formatted XML with indentation:

  \snippet qxmlstreamwriter/main.cpp start stream
  \dots
  \snippet qxmlstreamwriter/main.cpp write element
  \dots
  \snippet qxmlstreamwriter/main.cpp finish stream

  QXmlStreamWriter takes care of prefixing namespaces, all you have to
  do is specify the \c namespaceUri when writing elements or
  attributes. If you must conform to certain prefixes, you can force
  the writer to use them by declaring the namespaces manually with
  either writeNamespace() or writeDefaultNamespace(). Alternatively,
  you can bypass the stream writer's namespace support and use
  overloaded methods that take a qualified name instead. The namespace
  \e http://www.w3.org/XML/1998/namespace is implicit and mapped to the
  prefix \e xml.

  The stream writer can automatically format the generated XML data by
  adding line-breaks and indentation to empty sections between
  elements, making the XML data more readable for humans and easier to
  work with for most source code management systems. The feature can
  be turned on with the \l autoFormatting property, and customized
  with the \l autoFormattingIndent property.

  Other functions are writeCDATA(), writeComment(),
  writeProcessingInstruction(), and writeDTD(). Chaining of XML
  streams is supported with writeCurrentToken().

  QXmlStreamWriter always encodes XML in UTF-8.

  If an error occurs while writing to the underlying device, hasError()
  starts returning true and subsequent writes are ignored.

  The \l{QXmlStream Bookmarks Example} illustrates how to use a
  stream writer to write an XML bookmark file (XBEL) that
  was previously read in by a QXmlStreamReader.

*/

#ifndef QT_NO_XMLSTREAMWRITER

class QXmlStreamWriterPrivate : public QXmlStreamPrivateTagStack {
    QXmlStreamWriter *q_ptr;
    Q_DECLARE_PUBLIC(QXmlStreamWriter)
public:
    QXmlStreamWriterPrivate(QXmlStreamWriter *q);
    ~QXmlStreamWriterPrivate() {
        if (deleteDevice)
            delete device;
    }

    void write(const XmlStringRef &);
    void write(const QString &);
    void writeEscaped(const QString &, bool escapeWhitespace = false);
    void write(const char *s, qsizetype len);
    template <int N> void write(const char (&s)[N]) { write(s, N - 1); }
    bool finishStartElement(bool contents = true);
    void writeStartElement(const QString &namespaceUri, const QString &name);
    QIODevice *device;
    QString *stringDevice;
    uint deleteDevice :1;
    uint inStartElement :1;
    uint inEmptyElement :1;
    uint lastWasStartElement :1;
    uint wroteSomething :1;
    uint hasIoError :1;
    uint hasEncodingError :1;
    uint autoFormatting :1;
    QByteArray autoFormattingIndent;
    NamespaceDeclaration emptyNamespace;
    qsizetype lastNamespaceDeclaration;
    QStringEncoder toUtf8;

    NamespaceDeclaration &findNamespace(const QString &namespaceUri, bool writeDeclaration = false, bool noDefault = false);
    void writeNamespaceDeclaration(const NamespaceDeclaration &namespaceDeclaration);

    int namespacePrefixCount;

    void indent(int level);
};


QXmlStreamWriterPrivate::QXmlStreamWriterPrivate(QXmlStreamWriter *q)
    : autoFormattingIndent(4, ' '),
      toUtf8(QStringEncoder::Utf8, QStringEncoder::Flag::Stateless)
{
    q_ptr = q;
    device = nullptr;
    stringDevice = nullptr;
    deleteDevice = false;
    inStartElement = inEmptyElement = false;
    wroteSomething = false;
    hasIoError = false;
    hasEncodingError = false;
    lastWasStartElement = false;
    lastNamespaceDeclaration = 1;
    autoFormatting = false;
    namespacePrefixCount = 0;
}

void QXmlStreamWriterPrivate::write(const XmlStringRef &s)
{
    if (device) {
        if (hasIoError)
            return;
        QByteArray bytes = toUtf8(s);
        if (toUtf8.hasError()) {
            hasEncodingError = true;
            return;
        }
        if (device->write(bytes) != bytes.size())
            hasIoError = true;
    }
    else if (stringDevice)
        stringDevice->append(s);
    else
        qWarning("QXmlStreamWriter: No device");
}

void QXmlStreamWriterPrivate::write(const QString &s)
{
    if (device) {
        if (hasIoError)
            return;
        QByteArray bytes = toUtf8(s);
        if (toUtf8.hasError()) {
            hasEncodingError = true;
            return;
        }
        if (device->write(bytes) != bytes.size())
            hasIoError = true;
    }
    else if (stringDevice)
        stringDevice->append(s);
    else
        qWarning("QXmlStreamWriter: No device");
}

void QXmlStreamWriterPrivate::writeEscaped(const QString &s, bool escapeWhitespace)
{
    QString escaped;
    escaped.reserve(s.size());
    for (QChar c : s) {
        switch (c.unicode()) {
        case '<':
            escaped.append(QLatin1String("&lt;"));
            break;
        case '>':
            escaped.append(QLatin1String("&gt;"));
            break;
        case '&':
            escaped.append(QLatin1String("&amp;"));
            break;
        case '\"':
            escaped.append(QLatin1String("&quot;"));
            break;
        case '\t':
            if (escapeWhitespace)
                escaped.append(QLatin1String("&#9;"));
            else
                escaped += c;
            break;
        case '\n':
            if (escapeWhitespace)
                escaped.append(QLatin1String("&#10;"));
            else
                escaped += c;
            break;
        case '\v':
        case '\f':
            hasEncodingError = true;
            break;
        case '\r':
            if (escapeWhitespace)
                escaped.append(QLatin1String("&#13;"));
            else
                escaped += c;
            break;
        default:
            if (c.unicode() > 0x1f && c.unicode() < 0xfffe)
                escaped += c;
            else
                hasEncodingError = true;
            break;
        }
    }
    write(escaped);
}

// Writes utf8
void QXmlStreamWriterPrivate::write(const char *s, qsizetype len)
{
    if (device) {
        if (hasIoError)
            return;
        if (device->write(s, len) != len)
            hasIoError = true;
        return;
    }

    write(QString::fromUtf8(s, len));
}

void QXmlStreamWriterPrivate::writeNamespaceDeclaration(const NamespaceDeclaration &namespaceDeclaration) {
    if (namespaceDeclaration.prefix.isEmpty()) {
        write(" xmlns=\"");
        write(namespaceDeclaration.namespaceUri);
        write("\"");
    } else {
        write(" xmlns:");
        write(namespaceDeclaration.prefix);
        write("=\"");
        write(namespaceDeclaration.namespaceUri);
        write("\"");
    }
}

bool QXmlStreamWriterPrivate::finishStartElement(bool contents)
{
    bool hadSomethingWritten = wroteSomething;
    wroteSomething = contents;
    if (!inStartElement)
        return hadSomethingWritten;

    if (inEmptyElement) {
        write("/>");
        QXmlStreamWriterPrivate::Tag tag = tagStack_pop();
        lastNamespaceDeclaration = tag.namespaceDeclarationsSize;
        lastWasStartElement = false;
    } else {
        write(">");
    }
    inStartElement = inEmptyElement = false;
    lastNamespaceDeclaration = namespaceDeclarations.size();
    return hadSomethingWritten;
}

QXmlStreamPrivateTagStack::NamespaceDeclaration &QXmlStreamWriterPrivate::findNamespace(const QString &namespaceUri, bool writeDeclaration, bool noDefault)
{
    for (NamespaceDeclaration &namespaceDeclaration : reversed(namespaceDeclarations)) {
        if (namespaceDeclaration.namespaceUri == namespaceUri) {
            if (!noDefault || !namespaceDeclaration.prefix.isEmpty())
                return namespaceDeclaration;
        }
    }
    if (namespaceUri.isEmpty())
        return emptyNamespace;
    NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
    if (namespaceUri.isEmpty()) {
        namespaceDeclaration.prefix.clear();
    } else {
        QString s;
        int n = ++namespacePrefixCount;
        forever {
            s = QLatin1Char('n') + QString::number(n++);
            qsizetype j = namespaceDeclarations.size() - 2;
            while (j >= 0 && namespaceDeclarations.at(j).prefix != s)
                --j;
            if (j < 0)
                break;
        }
        namespaceDeclaration.prefix = addToStringStorage(s);
    }
    namespaceDeclaration.namespaceUri = addToStringStorage(namespaceUri);
    if (writeDeclaration)
        writeNamespaceDeclaration(namespaceDeclaration);
    return namespaceDeclaration;
}



void QXmlStreamWriterPrivate::indent(int level)
{
    write("\n");
    for (int i = level; i > 0; --i)
        write(autoFormattingIndent.constData(), autoFormattingIndent.length());
}


/*!
  Constructs a stream writer.

  \sa setDevice()
 */
QXmlStreamWriter::QXmlStreamWriter()
    : d_ptr(new QXmlStreamWriterPrivate(this))
{
}

/*!
  Constructs a stream writer that writes into \a device;
 */
QXmlStreamWriter::QXmlStreamWriter(QIODevice *device)
    : d_ptr(new QXmlStreamWriterPrivate(this))
{
    Q_D(QXmlStreamWriter);
    d->device = device;
}

/*!  Constructs a stream writer that writes into \a array. This is the
  same as creating an xml writer that operates on a QBuffer device
  which in turn operates on \a array.
 */
QXmlStreamWriter::QXmlStreamWriter(QByteArray *array)
    : d_ptr(new QXmlStreamWriterPrivate(this))
{
    Q_D(QXmlStreamWriter);
    d->device = new QBuffer(array);
    d->device->open(QIODevice::WriteOnly);
    d->deleteDevice = true;
}


/*!  Constructs a stream writer that writes into \a string.
 *
 */
QXmlStreamWriter::QXmlStreamWriter(QString *string)
    : d_ptr(new QXmlStreamWriterPrivate(this))
{
    Q_D(QXmlStreamWriter);
    d->stringDevice = string;
}

/*!
    Destructor.
*/
QXmlStreamWriter::~QXmlStreamWriter()
{
}


/*!
    Sets the current device to \a device. If you want the stream to
    write into a QByteArray, you can create a QBuffer device.

    \sa device()
*/
void QXmlStreamWriter::setDevice(QIODevice *device)
{
    Q_D(QXmlStreamWriter);
    if (device == d->device)
        return;
    d->stringDevice = nullptr;
    if (d->deleteDevice) {
        delete d->device;
        d->deleteDevice = false;
    }
    d->device = device;
}

/*!
    Returns the current device associated with the QXmlStreamWriter,
    or \nullptr if no device has been assigned.

    \sa setDevice()
*/
QIODevice *QXmlStreamWriter::device() const
{
    Q_D(const QXmlStreamWriter);
    return d->device;
}

/*!
    \property  QXmlStreamWriter::autoFormatting
    \since 4.4
    \brief the auto-formatting flag of the stream writer.

    This property controls whether or not the stream writer
    automatically formats the generated XML data. If enabled, the
    writer automatically adds line-breaks and indentation to empty
    sections between elements (ignorable whitespace). The main purpose
    of auto-formatting is to split the data into several lines, and to
    increase readability for a human reader. The indentation depth can
    be controlled through the \l autoFormattingIndent property.

    By default, auto-formatting is disabled.
*/

/*!
 \since 4.4

 Enables auto formatting if \a enable is \c true, otherwise
 disables it.

 The default value is \c false.
 */
void QXmlStreamWriter::setAutoFormatting(bool enable)
{
    Q_D(QXmlStreamWriter);
    d->autoFormatting = enable;
}

/*!
 \since 4.4

 Returns \c true if auto formatting is enabled, otherwise \c false.
 */
bool QXmlStreamWriter::autoFormatting() const
{
    Q_D(const QXmlStreamWriter);
    return d->autoFormatting;
}

/*!
    \property QXmlStreamWriter::autoFormattingIndent
    \since 4.4

    \brief the number of spaces or tabs used for indentation when
    auto-formatting is enabled.  Positive numbers indicate spaces,
    negative numbers tabs.

    The default indentation is 4.

    \sa autoFormatting
*/


void QXmlStreamWriter::setAutoFormattingIndent(int spacesOrTabs)
{
    Q_D(QXmlStreamWriter);
    d->autoFormattingIndent = QByteArray(qAbs(spacesOrTabs), spacesOrTabs >= 0 ? ' ' : '\t');
}

int QXmlStreamWriter::autoFormattingIndent() const
{
    Q_D(const QXmlStreamWriter);
    return d->autoFormattingIndent.count(' ') - d->autoFormattingIndent.count('\t');
}

/*!
    Returns \c true if writing failed.

    This can happen if the stream failed to write to the underlying
    device or if the data to be written contained invalid characters.

    The error status is never reset. Writes happening after the error
    occurred may be ignored, even if the error condition is cleared.
 */
bool QXmlStreamWriter::hasError() const
{
    Q_D(const QXmlStreamWriter);
    return d->hasIoError || d->hasEncodingError;
}

/*!
  \overload
  Writes an attribute with \a qualifiedName and \a value.


  This function can only be called after writeStartElement() before
  any content is written, or after writeEmptyElement().
 */
void QXmlStreamWriter::writeAttribute(const QString &qualifiedName, const QString &value)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(d->inStartElement);
    Q_ASSERT(qualifiedName.count(QLatin1Char(':')) <= 1);
    d->write(" ");
    d->write(qualifiedName);
    d->write("=\"");
    d->writeEscaped(value, true);
    d->write("\"");
}

/*!  Writes an attribute with \a name and \a value, prefixed for
  the specified \a namespaceUri. If the namespace has not been
  declared yet, QXmlStreamWriter will generate a namespace declaration
  for it.

  This function can only be called after writeStartElement() before
  any content is written, or after writeEmptyElement().
 */
void QXmlStreamWriter::writeAttribute(const QString &namespaceUri, const QString &name, const QString &value)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(d->inStartElement);
    Q_ASSERT(!name.contains(QLatin1Char(':')));
    QXmlStreamWriterPrivate::NamespaceDeclaration &namespaceDeclaration = d->findNamespace(namespaceUri, true, true);
    d->write(" ");
    if (!namespaceDeclaration.prefix.isEmpty()) {
        d->write(namespaceDeclaration.prefix);
        d->write(":");
    }
    d->write(name);
    d->write("=\"");
    d->writeEscaped(value, true);
    d->write("\"");
}

/*!
  \overload

  Writes the \a attribute.

  This function can only be called after writeStartElement() before
  any content is written, or after writeEmptyElement().
 */
void QXmlStreamWriter::writeAttribute(const QXmlStreamAttribute& attribute)
{
    if (attribute.namespaceUri().isEmpty())
        writeAttribute(attribute.qualifiedName().toString(),
                       attribute.value().toString());
    else
        writeAttribute(attribute.namespaceUri().toString(),
                       attribute.name().toString(),
                       attribute.value().toString());
}


/*!  Writes the attribute vector \a attributes. If a namespace
  referenced in an attribute not been declared yet, QXmlStreamWriter
  will generate a namespace declaration for it.

  This function can only be called after writeStartElement() before
  any content is written, or after writeEmptyElement().

  \sa writeAttribute(), writeNamespace()
 */
void QXmlStreamWriter::writeAttributes(const QXmlStreamAttributes& attributes)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(d->inStartElement);
    Q_UNUSED(d);
    for (const auto &attr : attributes)
        writeAttribute(attr);
}


/*!  Writes \a text as CDATA section. If \a text contains the
  forbidden character sequence "]]>", it is split into different CDATA
  sections.

  This function mainly exists for completeness. Normally you should
  not need use it, because writeCharacters() automatically escapes all
  non-content characters.
 */
void QXmlStreamWriter::writeCDATA(const QString &text)
{
    Q_D(QXmlStreamWriter);
    d->finishStartElement();
    QString copy(text);
    copy.replace(QLatin1String("]]>"), QLatin1String("]]]]><![CDATA[>"));
    d->write("<![CDATA[");
    d->write(copy);
    d->write("]]>");
}


/*!  Writes \a text. The characters "<", "&", and "\"" are escaped as entity
  references "&lt;", "&amp;, and "&quot;". To avoid the forbidden sequence
  "]]>", ">" is also escaped as "&gt;".

  \sa writeEntityReference()
 */
void QXmlStreamWriter::writeCharacters(const QString &text)
{
    Q_D(QXmlStreamWriter);
    d->finishStartElement();
    d->writeEscaped(text);
}


/*!  Writes \a text as XML comment, where \a text must not contain the
     forbidden sequence "--" or end with "-". Note that XML does not
     provide any way to escape "-" in a comment.
 */
void QXmlStreamWriter::writeComment(const QString &text)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(!text.contains(QLatin1String("--")) && !text.endsWith(QLatin1Char('-')));
    if (!d->finishStartElement(false) && d->autoFormatting)
        d->indent(d->tagStack.size());
    d->write("<!--");
    d->write(text);
    d->write("-->");
    d->inStartElement = d->lastWasStartElement = false;
}


/*!  Writes a DTD section. The \a dtd represents the entire
  doctypedecl production from the XML 1.0 specification.
 */
void QXmlStreamWriter::writeDTD(const QString &dtd)
{
    Q_D(QXmlStreamWriter);
    d->finishStartElement();
    if (d->autoFormatting)
        d->write("\n");
    d->write(dtd);
    if (d->autoFormatting)
        d->write("\n");
}



/*!  \overload
  Writes an empty element with qualified name \a qualifiedName.
  Subsequent calls to writeAttribute() will add attributes to this element.
*/
void QXmlStreamWriter::writeEmptyElement(const QString &qualifiedName)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(qualifiedName.count(QLatin1Char(':')) <= 1);
    d->writeStartElement(QString(), qualifiedName);
    d->inEmptyElement = true;
}


/*!  Writes an empty element with \a name, prefixed for the specified
  \a namespaceUri. If the namespace has not been declared,
  QXmlStreamWriter will generate a namespace declaration for it.
  Subsequent calls to writeAttribute() will add attributes to this element.

  \sa writeNamespace()
 */
void QXmlStreamWriter::writeEmptyElement(const QString &namespaceUri, const QString &name)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(!name.contains(QLatin1Char(':')));
    d->writeStartElement(namespaceUri, name);
    d->inEmptyElement = true;
}


/*!\overload
  Writes a text element with \a qualifiedName and \a text.


  This is a convenience function equivalent to:
  \snippet code/src_corelib_xml_qxmlstream.cpp 1

*/
void QXmlStreamWriter::writeTextElement(const QString &qualifiedName, const QString &text)
{
    writeStartElement(qualifiedName);
    writeCharacters(text);
    writeEndElement();
}

/*!  Writes a text element with \a name, prefixed for the specified \a
     namespaceUri, and \a text. If the namespace has not been
     declared, QXmlStreamWriter will generate a namespace declaration
     for it.


  This is a convenience function equivalent to:
  \snippet code/src_corelib_xml_qxmlstream.cpp 2

*/
void QXmlStreamWriter::writeTextElement(const QString &namespaceUri, const QString &name, const QString &text)
{
    writeStartElement(namespaceUri, name);
    writeCharacters(text);
    writeEndElement();
}


/*!
  Closes all remaining open start elements and writes a newline.

  \sa writeStartDocument()
 */
void QXmlStreamWriter::writeEndDocument()
{
    Q_D(QXmlStreamWriter);
    while (d->tagStack.size())
        writeEndElement();
    d->write("\n");
}

/*!
  Closes the previous start element.

  \sa writeStartElement()
 */
void QXmlStreamWriter::writeEndElement()
{
    Q_D(QXmlStreamWriter);
    if (d->tagStack.isEmpty())
        return;

    // shortcut: if nothing was written, close as empty tag
    if (d->inStartElement && !d->inEmptyElement) {
        d->write("/>");
        d->lastWasStartElement = d->inStartElement = false;
        QXmlStreamWriterPrivate::Tag tag = d->tagStack_pop();
        d->lastNamespaceDeclaration = tag.namespaceDeclarationsSize;
        return;
    }

    if (!d->finishStartElement(false) && !d->lastWasStartElement && d->autoFormatting)
        d->indent(d->tagStack.size()-1);
    if (d->tagStack.isEmpty())
        return;
    d->lastWasStartElement = false;
    QXmlStreamWriterPrivate::Tag tag = d->tagStack_pop();
    d->lastNamespaceDeclaration = tag.namespaceDeclarationsSize;
    d->write("</");
    if (!tag.namespaceDeclaration.prefix.isEmpty()) {
        d->write(tag.namespaceDeclaration.prefix);
        d->write(":");
    }
    d->write(tag.name);
    d->write(">");
}



/*!
  Writes the entity reference \a name to the stream, as "&\a{name};".
 */
void QXmlStreamWriter::writeEntityReference(const QString &name)
{
    Q_D(QXmlStreamWriter);
    d->finishStartElement();
    d->write("&");
    d->write(name);
    d->write(";");
}


/*!  Writes a namespace declaration for \a namespaceUri with \a
  prefix. If \a prefix is empty, QXmlStreamWriter assigns a unique
  prefix consisting of the letter 'n' followed by a number.

  If writeStartElement() or writeEmptyElement() was called, the
  declaration applies to the current element; otherwise it applies to
  the next child element.

  Note that the prefix \e xml is both predefined and reserved for
  \e http://www.w3.org/XML/1998/namespace, which in turn cannot be
  bound to any other prefix. The prefix \e xmlns and its URI
  \e http://www.w3.org/2000/xmlns/ are used for the namespace mechanism
  itself and thus completely forbidden in declarations.

 */
void QXmlStreamWriter::writeNamespace(const QString &namespaceUri, const QString &prefix)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(prefix != QLatin1String("xmlns"));
    if (prefix.isEmpty()) {
        d->findNamespace(namespaceUri, d->inStartElement);
    } else {
        Q_ASSERT(!((prefix == QLatin1String("xml")) ^ (namespaceUri == QLatin1String("http://www.w3.org/XML/1998/namespace"))));
        Q_ASSERT(namespaceUri != QLatin1String("http://www.w3.org/2000/xmlns/"));
        QXmlStreamWriterPrivate::NamespaceDeclaration &namespaceDeclaration = d->namespaceDeclarations.push();
        namespaceDeclaration.prefix = d->addToStringStorage(prefix);
        namespaceDeclaration.namespaceUri = d->addToStringStorage(namespaceUri);
        if (d->inStartElement)
            d->writeNamespaceDeclaration(namespaceDeclaration);
    }
}


/*! Writes a default namespace declaration for \a namespaceUri.

  If writeStartElement() or writeEmptyElement() was called, the
  declaration applies to the current element; otherwise it applies to
  the next child element.

  Note that the namespaces \e http://www.w3.org/XML/1998/namespace
  (bound to \e xmlns) and \e http://www.w3.org/2000/xmlns/ (bound to
  \e xml) by definition cannot be declared as default.
 */
void QXmlStreamWriter::writeDefaultNamespace(const QString &namespaceUri)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(namespaceUri != QLatin1String("http://www.w3.org/XML/1998/namespace"));
    Q_ASSERT(namespaceUri != QLatin1String("http://www.w3.org/2000/xmlns/"));
    QXmlStreamWriterPrivate::NamespaceDeclaration &namespaceDeclaration = d->namespaceDeclarations.push();
    namespaceDeclaration.prefix.clear();
    namespaceDeclaration.namespaceUri = d->addToStringStorage(namespaceUri);
    if (d->inStartElement)
        d->writeNamespaceDeclaration(namespaceDeclaration);
}


/*!
  Writes an XML processing instruction with \a target and \a data,
  where \a data must not contain the sequence "?>".
 */
void QXmlStreamWriter::writeProcessingInstruction(const QString &target, const QString &data)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(!data.contains(QLatin1String("?>")));
    if (!d->finishStartElement(false) && d->autoFormatting)
        d->indent(d->tagStack.size());
    d->write("<?");
    d->write(target);
    if (!data.isNull()) {
        d->write(" ");
        d->write(data);
    }
    d->write("?>");
}



/*!\overload

  Writes a document start with XML version number "1.0".

  \sa writeEndDocument()
  \since 4.5
 */
void QXmlStreamWriter::writeStartDocument()
{
    writeStartDocument(QLatin1String("1.0"));
}


/*!
  Writes a document start with the XML version number \a version.

  \sa writeEndDocument()
 */
void QXmlStreamWriter::writeStartDocument(const QString &version)
{
    Q_D(QXmlStreamWriter);
    d->finishStartElement(false);
    d->write("<?xml version=\"");
    d->write(version);
    if (d->device) // stringDevice does not get any encoding
        d->write("\" encoding=\"UTF-8");
    d->write("\"?>");
}

/*!  Writes a document start with the XML version number \a version
  and a standalone attribute \a standalone.

  \sa writeEndDocument()
  \since 4.5
 */
void QXmlStreamWriter::writeStartDocument(const QString &version, bool standalone)
{
    Q_D(QXmlStreamWriter);
    d->finishStartElement(false);
    d->write("<?xml version=\"");
    d->write(version);
    if (d->device) // stringDevice does not get any encoding
        d->write("\" encoding=\"UTF-8");
    if (standalone)
        d->write("\" standalone=\"yes\"?>");
    else
        d->write("\" standalone=\"no\"?>");
}


/*!\overload

   Writes a start element with \a qualifiedName. Subsequent calls to
   writeAttribute() will add attributes to this element.

   \sa writeEndElement(), writeEmptyElement()
 */
void QXmlStreamWriter::writeStartElement(const QString &qualifiedName)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(qualifiedName.count(QLatin1Char(':')) <= 1);
    d->writeStartElement(QString(), qualifiedName);
}


/*!  Writes a start element with \a name, prefixed for the specified
  \a namespaceUri. If the namespace has not been declared yet,
  QXmlStreamWriter will generate a namespace declaration for
  it. Subsequent calls to writeAttribute() will add attributes to this
  element.

  \sa writeNamespace(), writeEndElement(), writeEmptyElement()
 */
void QXmlStreamWriter::writeStartElement(const QString &namespaceUri, const QString &name)
{
    Q_D(QXmlStreamWriter);
    Q_ASSERT(!name.contains(QLatin1Char(':')));
    d->writeStartElement(namespaceUri, name);
}

void QXmlStreamWriterPrivate::writeStartElement(const QString &namespaceUri, const QString &name)
{
    if (!finishStartElement(false) && autoFormatting)
        indent(tagStack.size());

    Tag &tag = tagStack_push();
    tag.name = addToStringStorage(name);
    tag.namespaceDeclaration = findNamespace(namespaceUri);
    write("<");
    if (!tag.namespaceDeclaration.prefix.isEmpty()) {
        write(tag.namespaceDeclaration.prefix);
        write(":");
    }
    write(tag.name);
    inStartElement = lastWasStartElement = true;

    for (qsizetype i = lastNamespaceDeclaration; i < namespaceDeclarations.size(); ++i)
        writeNamespaceDeclaration(namespaceDeclarations[i]);
    tag.namespaceDeclarationsSize = lastNamespaceDeclaration;
}

#ifndef QT_NO_XMLSTREAMREADER
/*!  Writes the current state of the \a reader. All possible valid
  states are supported.

  The purpose of this function is to support chained processing of XML data.

  \sa QXmlStreamReader::tokenType()
 */
void QXmlStreamWriter::writeCurrentToken(const QXmlStreamReader &reader)
{
    switch (reader.tokenType()) {
    case QXmlStreamReader::NoToken:
        break;
    case QXmlStreamReader::StartDocument:
        writeStartDocument();
        break;
    case QXmlStreamReader::EndDocument:
        writeEndDocument();
        break;
    case QXmlStreamReader::StartElement: {
        writeStartElement(reader.namespaceUri().toString(), reader.name().toString());
        const QXmlStreamNamespaceDeclarations decls = reader.namespaceDeclarations();
        for (const auto &namespaceDeclaration : decls) {
            writeNamespace(namespaceDeclaration.namespaceUri().toString(),
                           namespaceDeclaration.prefix().toString());
        }
        writeAttributes(reader.attributes());
             } break;
    case QXmlStreamReader::EndElement:
        writeEndElement();
        break;
    case QXmlStreamReader::Characters:
        if (reader.isCDATA())
            writeCDATA(reader.text().toString());
        else
            writeCharacters(reader.text().toString());
        break;
    case QXmlStreamReader::Comment:
        writeComment(reader.text().toString());
        break;
    case QXmlStreamReader::DTD:
        writeDTD(reader.text().toString());
        break;
    case QXmlStreamReader::EntityReference:
        writeEntityReference(reader.name().toString());
        break;
    case QXmlStreamReader::ProcessingInstruction:
        writeProcessingInstruction(reader.processingInstructionTarget().toString(),
                                   reader.processingInstructionData().toString());
        break;
    default:
        Q_ASSERT(reader.tokenType() != QXmlStreamReader::Invalid);
        qWarning("QXmlStreamWriter: writeCurrentToken() with invalid state.");
        break;
    }
}

/*!
 \fn bool QXmlStreamAttributes::hasAttribute(const QString &qualifiedName) const
 \since 4.5

 Returns \c true if this QXmlStreamAttributes has an attribute whose
 qualified name is \a qualifiedName; otherwise returns \c false.

 Note that this is not namespace aware. For instance, if this
 QXmlStreamAttributes contains an attribute whose lexical name is "xlink:href"
 this doesn't tell that an attribute named \c href in the XLink namespace is
 present, since the \c xlink prefix can be bound to any namespace. Use the
 overload that takes a namespace URI and a local name as parameter, for
 namespace aware code.
*/

/*!
 \fn bool QXmlStreamAttributes::hasAttribute(QLatin1String qualifiedName) const
 \overload
 \since 4.5
*/

/*!
 \fn bool QXmlStreamAttributes::hasAttribute(const QString &namespaceUri,
                                             const QString &name) const
 \overload
 \since 4.5

 Returns \c true if this QXmlStreamAttributes has an attribute whose
 namespace URI and name correspond to \a namespaceUri and \a name;
 otherwise returns \c false.
*/

#endif // QT_NO_XMLSTREAMREADER
#endif // QT_NO_XMLSTREAMWRITER

QT_END_NAMESPACE

#endif // QT_NO_XMLSTREAM
