// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

//
//  W A R N I N G
//  -------------
//
// This file is automatically generated from qxmlstream.g.
// Changes should be made to that file, not here. Any change to this file will
// be lost!
//
// To regenerate this file, run:
//    qlalr --no-debug --no-lines --qt qxmlstream.g
//

#include <QtCore/private/qglobal_p.h>
#include <qxmlstream.h>
#include "qxmlstream_p.h"
#include "qxmlutils_p.h"
#include <qstringconverter.h>

#include <memory>

#ifndef QXMLSTREAMPARSER_P_H
#define QXMLSTREAMPARSER_P_H

QT_BEGIN_NAMESPACE

#if QT_CONFIG(xmlstreamreader)

bool QXmlStreamReaderPrivate::parse()
{
    // cleanup currently reported token

    using namespace Qt::StringLiterals;

    switch (type) {
    case QXmlStreamReader::StartElement:
        name.clear();
        prefix.clear();
        qualifiedName.clear();
        namespaceUri.clear();
        publicNamespaceDeclarations.clear();
        attributes.clear();
        if (isEmptyElement) {
            setType(QXmlStreamReader::EndElement);
            Tag tag = tagStack_pop();
            namespaceUri = tag.namespaceDeclaration.namespaceUri;
            prefix = tag.namespaceDeclaration.prefix;
            name = tag.name;
            qualifiedName = tag.qualifiedName;
            isEmptyElement = false;
            return true;
        }
        clearTextBuffer();
        break;
    case QXmlStreamReader::EndElement:
        name.clear();
        prefix.clear();
        qualifiedName.clear();
        namespaceUri.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::DTD:
        publicNotationDeclarations.clear();
        publicEntityDeclarations.clear();
        dtdName.clear();
        dtdPublicId.clear();
        dtdSystemId.clear();
        Q_FALLTHROUGH();
    case QXmlStreamReader::Comment:
    case QXmlStreamReader::Characters:
        isCDATA = false;
        isWhitespace = true;
        text.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::EntityReference:
        text.clear();
        name.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::ProcessingInstruction:
        processingInstructionTarget.clear();
        processingInstructionData.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::NoToken:
    case QXmlStreamReader::Invalid:
        break;
    case QXmlStreamReader::StartDocument:
        lockEncoding = true;
        documentVersion.clear();
        documentEncoding.clear();
        if (decoder.isValid() && decoder.hasError()) {
            raiseWellFormedError(QXmlStream::tr("Encountered incorrectly encoded content."));
            readBuffer.clear();
            return false;
        }
        Q_FALLTHROUGH();
    default:
        clearTextBuffer();
        ;
    }

    setType(QXmlStreamReader::NoToken);


    // the main parse loop
    int act, r;

    if (resumeReduction) {
        act = state_stack[tos-1];
        r = resumeReduction;
        resumeReduction = 0;
        goto ResumeReduction;
    }

    act = state_stack[tos];

    forever {
        if (token == -1 && - TERMINAL_COUNT != action_index[act]) {
            uint cu = getChar();
            token = NOTOKEN;
            token_char = cu == ~0U ? cu : ushort(cu);
            if ((cu != ~0U) && (cu & 0xff0000)) {
                token = cu >> 16;
            } else switch (token_char) {
            case 0xfffe:
            case 0xffff:
                token = XML_ERROR;
                break;
            case '\r':
                token = SPACE;
                if (cu == '\r') {
                    if ((token_char = filterCarriageReturn())) {
                        ++lineNumber;
                        lastLineStart = characterOffset + readBufferPos;
                        break;
                    }
                } else {
                    break;
                }
                Q_FALLTHROUGH();
            case ~0U: {
                token = EOF_SYMBOL;
                if (!tagsDone && !inParseEntity) {
                    int a = t_action(act, token);
                    if (a < 0) {
                        raiseError(QXmlStreamReader::PrematureEndOfDocumentError);
                        return false;
                    }
                }

            } break;
            case '\n':
                ++lineNumber;
                lastLineStart = characterOffset + readBufferPos;
                Q_FALLTHROUGH();
            case ' ':
            case '\t':
                token = SPACE;
                break;
            case '&':
                token = AMPERSAND;
                break;
            case '#':
                token = HASH;
                break;
            case '\'':
                token = QUOTE;
                break;
            case '\"':
                token = DBLQUOTE;
                break;
            case '<':
                token = LANGLE;
                break;
            case '>':
                token = RANGLE;
                break;
            case '[':
                token = LBRACK;
                break;
            case ']':
                token = RBRACK;
                break;
            case '(':
                token = LPAREN;
                break;
            case ')':
                token = RPAREN;
                break;
            case '|':
                token = PIPE;
                break;
            case '=':
                token = EQ;
                break;
            case '%':
                token = PERCENT;
                break;
            case '/':
                token = SLASH;
                break;
            case ':':
                token = COLON;
                break;
            case ';':
                token = SEMICOLON;
                break;
            case ',':
                token = COMMA;
                break;
            case '-':
                token = DASH;
                break;
            case '+':
                token = PLUS;
                break;
            case '*':
                token = STAR;
                break;
            case '.':
                token = DOT;
                break;
            case '?':
                token = QUESTIONMARK;
                break;
            case '!':
                token = BANG;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                token = DIGIT;
                break;
            default:
                if (cu < 0x20)
                    token = NOTOKEN;
                else
                    token = LETTER;
                break;
            }
        }

        act = t_action (act, token);
        if (act == ACCEPT_STATE) {
            // reset the parser in case someone resumes (process instructions can follow a valid document)
            tos = 0;
            state_stack[tos++] = 0;
            state_stack[tos] = 0;
            return true;
        } else if (act > 0) {
            if (++tos >= stack_size-1)
                reallocateStack();

            Value &val = sym_stack[tos];
            val.c = token_char;
            val.pos = textBuffer.size();
            val.prefix = 0;
            val.len = 1;
            if (token_char)
                textBuffer += QChar(token_char);

            state_stack[tos] = act;
            token = -1;


        } else if (act < 0) {
            r = - act - 1;

#if defined (QLALR_DEBUG)
            int ridx = rule_index[r];
            printf ("%3d) %s ::=", r + 1, spell[rule_info[ridx]]);
            ++ridx;
            for (int i = ridx; i < ridx + rhs[r]; ++i) {
                int symbol = rule_info[i];
                if (const char *name = spell[symbol])
                    printf (" %s", name);
                else
                    printf (" #%d", symbol);
            }
            printf ("\n");
#endif

            tos -= rhs[r];
            act = state_stack[tos++];
        ResumeReduction:
            switch (r) {

        case 0:
            setType(QXmlStreamReader::EndDocument);
        break;

        case 1:
            if (type != QXmlStreamReader::Invalid) {
                if (hasSeenTag || inParseEntity) {
                    setType(QXmlStreamReader::EndDocument);
                } else {
                    raiseError(QXmlStreamReader::NotWellFormedError, QXmlStream::tr("Start tag expected."));
                    // reset the parser
                    tos = 0;
                    state_stack[tos++] = 0;
                    state_stack[tos] = 0;
                    return false;
                }
            }
        break;

        case 10: {
            auto reference = entityReferenceStack.pop();
            auto it = reference.hash->find(reference.name);
            Q_ASSERT(it != reference.hash->end());
            it->isCurrentlyReferenced = false;
            if (entityReferenceStack.isEmpty())
                entityLength = 0;
            clearSym();
        } break;

        case 11:
            if (!scanString(spell[VERSION], VERSION, false) && atEnd) {
                resume(11);
                return false;
            }
        break;

        case 12:
            setType(QXmlStreamReader::StartDocument);
            documentVersion = symString(6);
            startDocument();
        break;

        case 13:
            hasExternalDtdSubset = true;
            dtdSystemId = symString(2);
        break;

        case 14:
            checkPublicLiteral(symString(2));
            dtdPublicId = symString(2);
            dtdSystemId = symString(4);
            hasExternalDtdSubset = true;
        break;

        case 16:
            if (!scanPublicOrSystem() && atEnd) {
                resume(16);
                return false;
            }
            dtdName = symString(3);
        break;

        case 17:
        case 18:
            dtdName = symString(3);
            Q_FALLTHROUGH();

        case 19:
        case 20:
            setType(QXmlStreamReader::DTD);
            text = &textBuffer;
        break;

        case 21:
            scanDtd = true;
        break;

        case 22:
            scanDtd = false;
        break;

        case 37:
            if (!scanString(spell[EMPTY], EMPTY, false)
                && !scanString(spell[ANY], ANY, false)
                && atEnd) {
                resume(37);
                return false;
            }
        break;

        case 43:
            if (!scanString(spell[PCDATA], PCDATA, false) && atEnd) {
                resume(43);
                return false;
            }
        break;

        case 68: {
            lastAttributeIsCData = true;
        } break;

        case 78:
            if (!scanAfterDefaultDecl() && atEnd) {
                resume(78);
                return false;
            }
        break;

        case 83:
                sym(1) = sym(2);
                lastAttributeValue.clear();
                lastAttributeIsCData = false;
                if (!scanAttType() && atEnd) {
                    resume(83);
                    return false;
                }
        break;

        case 84: {
            DtdAttribute &dtdAttribute = dtdAttributes.push();
            dtdAttribute.tagName.clear();
            dtdAttribute.isCDATA = lastAttributeIsCData;
            dtdAttribute.attributePrefix = addToStringStorage(symPrefix(1));
            dtdAttribute.attributeName = addToStringStorage(symString(1));
            dtdAttribute.attributeQualifiedName = addToStringStorage(symName(1));
            dtdAttribute.isNamespaceAttribute = (dtdAttribute.attributePrefix == "xmlns"_L1
                                                 || (dtdAttribute.attributePrefix.isEmpty()
                                                     && dtdAttribute.attributeName == "xmlns"_L1));
            if (lastAttributeValue.isNull()) {
                dtdAttribute.defaultValue.clear();
            } else {
                if (dtdAttribute.isCDATA)
                    dtdAttribute.defaultValue = addToStringStorage(lastAttributeValue);
                else
                    dtdAttribute.defaultValue = addToStringStorage(lastAttributeValue.toString().simplified());

            }
        } break;

        case 88: {
            if (referenceToUnparsedEntityDetected && !standalone)
                break;
            qsizetype n = dtdAttributes.size();
            XmlStringRef tagName = addToStringStorage(symName(3));
            while (n--) {
                DtdAttribute &dtdAttribute = dtdAttributes[n];
                if (!dtdAttribute.tagName.isNull())
                    break;
                dtdAttribute.tagName = tagName;
                for (qsizetype i = 0; i < n; ++i) {
                    if ((dtdAttributes[i].tagName.isNull() || dtdAttributes[i].tagName == tagName)
                        && dtdAttributes[i].attributeQualifiedName == dtdAttribute.attributeQualifiedName) {
                        dtdAttribute.attributeQualifiedName.clear(); // redefined, delete it
                        break;
                    }
                }
            }
        } break;

        case 89: {
            if (!scanPublicOrSystem() && atEnd) {
                resume(89);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.push();
            entityDeclaration.clear();
            entityDeclaration.name = symString(3);
        } break;

        case 90: {
            if (!scanPublicOrSystem() && atEnd) {
                resume(90);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.push();
            entityDeclaration.clear();
            entityDeclaration.name = symString(5);
            entityDeclaration.parameter = true;
        } break;

        case 91: {
            if (!scanNData() && atEnd) {
                resume(91);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            entityDeclaration.systemId = symString(3);
            entityDeclaration.external = true;
        } break;

        case 92: {
            if (!scanNData() && atEnd) {
                resume(92);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            checkPublicLiteral((entityDeclaration.publicId = symString(3)));
            entityDeclaration.systemId = symString(5);
            entityDeclaration.external = true;
        } break;

        case 93: {
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            entityDeclaration.notationName = symString(3);
            if (entityDeclaration.parameter)
                raiseWellFormedError(QXmlStream::tr("NDATA in parameter entity declaration."));
        }
        Q_FALLTHROUGH();

        case 94:
        case 95: {
            if (referenceToUnparsedEntityDetected && !standalone) {
                entityDeclarations.pop();
                break;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            if (!entityDeclaration.external)
                entityDeclaration.value = symString(2);
            auto &hash = entityDeclaration.parameter ? parameterEntityHash : entityHash;
            if (!hash.contains(entityDeclaration.name)) {
                Entity entity(entityDeclaration.name.toString(),
                              entityDeclaration.value.toString());
                entity.unparsed = (!entityDeclaration.notationName.isNull());
                entity.external = entityDeclaration.external;
                hash.insert(qToStringViewIgnoringNull(entity.name), entity);
            }
        } break;

        case 96: {
            setType(QXmlStreamReader::ProcessingInstruction);
            const qsizetype pos = sym(4).pos + sym(4).len;
            processingInstructionTarget = symString(3);
            if (scanUntil("?>")) {
                processingInstructionData = XmlStringRef(&textBuffer, pos, textBuffer.size() - pos - 2);
                if (!processingInstructionTarget.view().compare("xml"_L1, Qt::CaseInsensitive)) {
                    raiseWellFormedError(QXmlStream::tr("XML declaration not at start of document."));
                }
                else if (!QXmlUtils::isNCName(processingInstructionTarget))
                    raiseWellFormedError(QXmlStream::tr("%1 is an invalid processing instruction name.")
                                         .arg(processingInstructionTarget));
            } else if (type != QXmlStreamReader::Invalid){
                resume(96);
                return false;
            }
        } break;

        case 97:
            setType(QXmlStreamReader::ProcessingInstruction);
            processingInstructionTarget = symString(3);
            if (!processingInstructionTarget.view().compare("xml"_L1, Qt::CaseInsensitive))
                raiseWellFormedError(QXmlStream::tr("Invalid processing instruction name."));
        break;

        case 98:
            if (!scanAfterLangleBang() && atEnd) {
                resume(98);
                return false;
            }
        break;

        case 99:
            if (!scanUntil("--")) {
                resume(99);
                return false;
            }
        break;

        case 100: {
            setType(QXmlStreamReader::Comment);
            const qsizetype pos = sym(1).pos + 4;
            text = XmlStringRef(&textBuffer, pos, textBuffer.size() - pos - 3);
        } break;

        case 101: {
            setType(QXmlStreamReader::Characters);
            isCDATA = true;
            isWhitespace = false;
            const qsizetype pos = sym(2).pos;
            if (scanUntil("]]>", -1)) {
                text = XmlStringRef(&textBuffer, pos, textBuffer.size() - pos - 3);
            } else {
                resume(101);
                return false;
            }
        } break;

        case 102: {
            if (!scanPublicOrSystem() && atEnd) {
                resume(102);
                return false;
            }
            NotationDeclaration &notationDeclaration = notationDeclarations.push();
            notationDeclaration.name = symString(3);
        } break;

        case 103: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            notationDeclaration.systemId = symString(3);
            notationDeclaration.publicId.clear();
        } break;

        case 104: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            notationDeclaration.systemId.clear();
            checkPublicLiteral((notationDeclaration.publicId = symString(3)));
        } break;

        case 105: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            checkPublicLiteral((notationDeclaration.publicId = symString(3)));
            notationDeclaration.systemId = symString(5);
        } break;

        case 129:
            isWhitespace = false;
            Q_FALLTHROUGH();

        case 130:
            sym(1).len += fastScanContentCharList();
            if (atEnd && !inParseEntity) {
                resume(130);
                return false;
            }
        break;

        case 139:
            if (!textBuffer.isEmpty()) {
                setType(QXmlStreamReader::Characters);
                text = &textBuffer;
            }
        break;

        case 140:
        case 141:
            clearSym();
        break;

        case 142:
        case 143:
            sym(1) = sym(2);
        break;

        case 144:
        case 145:
        case 146:
        case 147:
            sym(1).len += sym(2).len;
        break;

        case 173:
            if (normalizeLiterals)
                textBuffer.data()[textBuffer.size()-1] = u' ';
        break;

        case 174:
            sym(1).len += fastScanLiteralContent();
            if (atEnd) {
                resume(174);
                return false;
            }
        break;

        case 175: {
            if (!QXmlUtils::isPublicID(symString(1))) {
                raiseWellFormedError(QXmlStream::tr("%1 is an invalid PUBLIC identifier.").arg(symString(1)));
                resume(175);
                return false;
            }
        } break;

        case 176:
        case 177:
            clearSym();
        break;

        case 178:
        case 179:
            sym(1) = sym(2);
        break;

        case 180:
        case 181:
        case 182:
        case 183:
            sym(1).len += sym(2).len;
        break;

        case 213:
        case 214:
            clearSym();
        break;

        case 215:
        case 216:
            sym(1) = sym(2);
            lastAttributeValue = symString(1);
        break;

        case 217:
        case 218:
        case 219:
        case 220:
            sym(1).len += sym(2).len;
        break;

        case 229: {
            XmlStringRef prefix = symPrefix(1);
            if (prefix.isEmpty() && symString(1) == "xmlns"_L1 && namespaceProcessing) {
                NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                namespaceDeclaration.prefix.clear();

                const XmlStringRef ns(symString(5));
                if (ns.view() == "http://www.w3.org/2000/xmlns/"_L1 ||
                    ns.view() == "http://www.w3.org/XML/1998/namespace"_L1)
                    raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));
                else
                    namespaceDeclaration.namespaceUri = addToStringStorage(ns);
            } else {
                Attribute &attribute = attributeStack.push();
                attribute.key = sym(1);
                attribute.value = sym(5);

                XmlStringRef attributeQualifiedName = symName(1);
                bool normalize = false;
                for (const DtdAttribute &dtdAttribute : std::as_const(dtdAttributes)) {
                    if (!dtdAttribute.isCDATA
                        && dtdAttribute.tagName == qualifiedName
                        && dtdAttribute.attributeQualifiedName == attributeQualifiedName
                        ) {
                        normalize = true;
                        break;
                    }
                }
                if (normalize) {
                    // normalize attribute value (simplify and trim)
                    const qsizetype pos = textBuffer.size();
                    qsizetype n = 0;
                    bool wasSpace = true;
                    for (qsizetype i = 0; i < attribute.value.len; ++i) {
                        QChar c = textBuffer.at(attribute.value.pos + i);
                        if (c.unicode() == ' ') {
                            if (wasSpace)
                                continue;
                            wasSpace = true;
                        } else {
                            wasSpace = false;
                        }
                        textBuffer += textBuffer.at(attribute.value.pos + i);
                        ++n;
                    }
                    if (wasSpace)
                        while (n && textBuffer.at(pos + n - 1).unicode() == ' ')
                            --n;
                    attribute.value.pos = pos;
                    attribute.value.len = n;
                }
                if (prefix == "xmlns"_L1 && namespaceProcessing) {
                    NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                    XmlStringRef namespacePrefix = symString(attribute.key);
                    XmlStringRef namespaceUri = symString(attribute.value);
                    attributeStack.pop();
                    if (((namespacePrefix == "xml"_L1)
                         ^ (namespaceUri == "http://www.w3.org/XML/1998/namespace"_L1))
                        || namespaceUri == "http://www.w3.org/2000/xmlns/"_L1
                        || namespaceUri.isEmpty()
                        || namespacePrefix == "xmlns"_L1)
                        raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));

                    namespaceDeclaration.prefix = addToStringStorage(namespacePrefix);
                    namespaceDeclaration.namespaceUri = addToStringStorage(namespaceUri);
                }
            }
        } break;

        case 235: {
            normalizeLiterals = true;
            Tag &tag = tagStack_push();
            prefix = tag.namespaceDeclaration.prefix  = addToStringStorage(symPrefix(2));
            name = tag.name = addToStringStorage(symString(2));
            qualifiedName = tag.qualifiedName = addToStringStorage(symName(2));
            if ((!prefix.isEmpty() && !QXmlUtils::isNCName(prefix)) || !QXmlUtils::isNCName(name))
                raiseWellFormedError(QXmlStream::tr("Invalid XML name."));
        } break;

        case 236:
            isEmptyElement = true;
            Q_FALLTHROUGH();

        case 237:
            setType(QXmlStreamReader::StartElement);
            resolveTag();
            if (tagStack.size() == 1 && hasSeenTag && !inParseEntity)
                raiseWellFormedError(QXmlStream::tr("Extra content at end of document."));
            hasSeenTag = true;
        break;

        case 238: {
            setType(QXmlStreamReader::EndElement);
            Tag tag = tagStack_pop();

            namespaceUri = tag.namespaceDeclaration.namespaceUri;
            prefix = tag.namespaceDeclaration.prefix;
            name = tag.name;
            qualifiedName = tag.qualifiedName;
            if (qualifiedName != symName(3))
                raiseWellFormedError(QXmlStream::tr("Opening and ending tag mismatch."));
        } break;

        case 239:
            if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(unresolvedEntity));
                break;
            }
            setType(QXmlStreamReader::EntityReference);
            name = &unresolvedEntity;
        break;

        case 240: {
            sym(1).len += sym(2).len + 1;
            QStringView reference = symView(2);
            if (const auto it = entityHash.find(reference); it != entityHash.end()) {
                Entity &entity = *it;
                if (entity.unparsed) {
                    raiseWellFormedError(QXmlStream::tr("Reference to unparsed entity '%1'.").arg(reference));
                } else {
                    if (!entity.hasBeenParsed) {
                        parseEntity(entity.value);
                        entity.hasBeenParsed = true;
                    }
                    if (entity.literal)
                        putStringLiteral(entity.value);
                    else if (referenceEntity(&entityHash, entity))
                        putReplacement(entity.value);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                }
                break;
            }

            if (entityResolver) {
                QString replacementText = resolveUndeclaredEntity(reference.toString());
                if (!replacementText.isNull()) {
                    putReplacement(replacementText);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                    break;
                }
            }

            injectToken(UNRESOLVED_ENTITY);
            unresolvedEntity = symString(2).toString();
            textBuffer.chop(2 + sym(2).len);
            clearSym();

        } break;

        case 241: {
            sym(1).len += sym(2).len + 1;
            QStringView reference = symView(2);
            if (const auto it = parameterEntityHash.find(reference); it != parameterEntityHash.end()) {
                referenceToParameterEntityDetected = true;
                Entity &entity = *it;
                if (entity.unparsed || entity.external) {
                    referenceToUnparsedEntityDetected = true;
                } else {
                    if (referenceEntity(&parameterEntityHash, entity))
                        putString(entity.value);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                }
            } else if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(symString(2)));
            }
        } break;

        case 242:
            sym(1).len += sym(2).len + 1;
        break;

        case 243: {
            sym(1).len += sym(2).len + 1;
            QStringView reference = symView(2);
            if (const auto it = entityHash.find(reference); it != entityHash.end()) {
                Entity &entity = *it;
                if (entity.unparsed || entity.value.isNull()) {
                    raiseWellFormedError(QXmlStream::tr("Reference to external entity '%1' in attribute value.").arg(reference));
                    break;
                }
                if (!entity.hasBeenParsed) {
                    parseEntity(entity.value);
                    entity.hasBeenParsed = true;
                }
                if (entity.literal)
                    putStringLiteral(entity.value);
                else if (referenceEntity(&entityHash, entity))
                    putReplacementInAttributeValue(entity.value);
                textBuffer.chop(2 + sym(2).len);
                clearSym();
                break;
            }

            if (entityResolver) {
                QString replacementText = resolveUndeclaredEntity(reference.toString());
                if (!replacementText.isNull()) {
                    putReplacement(replacementText);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                    break;
                }
            }
            if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(reference));
            }
        } break;

        case 244: {
            if (char32_t s = resolveCharRef(3)) {
                putStringLiteral(QChar::fromUcs4(s));
                textBuffer.chop(3 + sym(3).len);
                clearSym();
            } else {
                raiseWellFormedError(QXmlStream::tr("Invalid character reference."));
            }
        } break;

        case 247:
        case 248:
            sym(1).len += sym(2).len;
        break;

        case 259:
            sym(1).len += fastScanSpace();
            if (atEnd) {
                resume(259);
                return false;
            }
        break;

        case 262: {
            Value &val = sym(1);
            val.len += fastScanName(&val);
            if (atEnd) {
                resume(262);
                return false;
            }
        } break;

        case 263:
            sym(1).len += fastScanName();
            if (atEnd) {
                resume(263);
                return false;
            }
        break;

        case 264:
        case 265:
        case 266:
        case 267:
        case 268:
            sym(1).len += fastScanNMTOKEN();
            if (atEnd) {
                resume(268);
                return false;
            }

        break;

    default:
        ;
    } // switch
            act = state_stack[tos] = nt_action (act, lhs[r] - TERMINAL_COUNT);
            if (type != QXmlStreamReader::NoToken)
                return true;
        } else {
            parseError();
            break;
        }
    }
    return false;
}

#endif // feature xmlstreamreader

QT_END_NAMESPACE

#endif

