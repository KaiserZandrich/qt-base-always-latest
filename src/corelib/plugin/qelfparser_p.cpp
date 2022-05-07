/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qelfparser_p.h"

#if defined (Q_OF_ELF) && defined(Q_CC_GNU)

#include "qlibrary_p.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

// #define QELFPARSER_DEBUG 1

const char *QElfParser::parseSectionHeader(const char *data, ElfSectionHeader *sh)
{
    sh->name = qFromUnaligned<qelfword_t>(data);
    data += sizeof(qelfword_t); // sh_name
    sh->type = qFromUnaligned<qelfword_t>(data);
    data += sizeof(qelfword_t)  // sh_type
         + sizeof(qelfaddr_t)   // sh_flags
         + sizeof(qelfaddr_t);  // sh_addr
    sh->offset = qFromUnaligned<qelfoff_t>(data);
    data += sizeof(qelfoff_t);  // sh_offset
    sh->size = qFromUnaligned<qelfoff_t>(data);
    data += sizeof(qelfoff_t);  // sh_size
    return data;
}

auto QElfParser::parse(const char *dataStart, ulong fdlen, const QString &library,
                       QLibraryPrivate *lib, qsizetype *pos, qsizetype *sectionlen) -> ScanResult
{
#if defined(QELFPARSER_DEBUG)
    qDebug() << "QElfParser::parse " << library;
#endif

    if (fdlen < 64) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is not an ELF object (%2)").arg(library, QLibrary::tr("file too small"));
        return NotElf;
    }
    const char *data = dataStart;
    if (qstrncmp(data, "\177ELF", 4) != 0) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is not an ELF object").arg(library);
        return NotElf;
    }
    // 32 or 64 bit
    if (data[4] != 1 && data[4] != 2) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)").arg(library, QLibrary::tr("odd cpu architecture"));
        return Corrupt;
    }

    /*  If you remove this check, to read ELF objects of a different arch, please make sure you modify the typedefs
        to match the _plugin_ architecture.
    */
    constexpr int ExpectedClass = (sizeof(void *) == 4) ? 1 : 2;
    if (data[4] != ExpectedClass) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)").arg(library, QLibrary::tr("wrong cpu architecture"));
        return Corrupt;
    }

    // endian
    constexpr int ExpectedEndianness = (Q_BYTE_ORDER == Q_LITTLE_ENDIAN) ? 1 : 2;
    if (data[5] != ExpectedEndianness) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)").arg(library, QLibrary::tr("odd endianness"));
        return Corrupt;
    }

    data += 16                  // e_ident
         +  sizeof(qelfhalf_t)  // e_type
         +  sizeof(qelfhalf_t)  // e_machine
         +  sizeof(qelfword_t)  // e_version
         +  sizeof(qelfaddr_t)  // e_entry
         +  sizeof(qelfoff_t);  // e_phoff

    qelfoff_t e_shoff = qFromUnaligned<qelfoff_t> (data);
    data += sizeof(qelfoff_t)    // e_shoff
         +  sizeof(qelfword_t);  // e_flags

    qelfhalf_t e_shsize = qFromUnaligned<qelfhalf_t> (data);

    if (e_shsize > fdlen) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)").arg(library, QLibrary::tr("unexpected e_shsize"));
        return Corrupt;
    }

    data += sizeof(qelfhalf_t)  // e_ehsize
         +  sizeof(qelfhalf_t)  // e_phentsize
         +  sizeof(qelfhalf_t); // e_phnum

    qelfhalf_t e_shentsize = qFromUnaligned<qelfhalf_t> (data);

    if (e_shentsize % 4) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)").arg(library, QLibrary::tr("unexpected e_shentsize"));
        return Corrupt;
    }
    data += sizeof(qelfhalf_t); // e_shentsize
    qelfhalf_t e_shnum     = qFromUnaligned<qelfhalf_t> (data);
    data += sizeof(qelfhalf_t); // e_shnum
    qelfhalf_t e_shtrndx   = qFromUnaligned<qelfhalf_t> (data);
    data += sizeof(qelfhalf_t); // e_shtrndx

    if ((quint32)(e_shnum * e_shentsize) > fdlen) {
        if (lib) {
            const QString message =
                QLibrary::tr("announced %n section(s), each %1 byte(s), exceed file size",
                             nullptr, int(e_shnum)).arg(e_shentsize);
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)").arg(library, message);
        }
        return Corrupt;
    }

#if defined(QELFPARSER_DEBUG)
    qDebug() << e_shnum << "sections starting at " << ("0x" + QByteArray::number(e_shoff, 16)).data() << "each" << e_shentsize << "bytes";
#endif

    ElfSectionHeader strtab;
    qulonglong soff = e_shoff + qelfword_t(e_shentsize) * qelfword_t(e_shtrndx);

    if ((soff + e_shentsize) > fdlen || soff % 4 || soff == 0) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                               .arg(library, QLibrary::tr("shstrtab section header seems to be at %1")
                                             .arg(QString::number(soff, 16)));
        return Corrupt;
    }

    parseSectionHeader(dataStart + soff, &strtab);
    m_stringTableFileOffset = strtab.offset;

    if ((quint32)(strtab.offset + strtab.size) > fdlen || strtab.offset == 0) {
        if (lib)
            lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                               .arg(library, QLibrary::tr("string table seems to be at %1")
                                             .arg(QString::number(strtab.offset, 16)));
        return Corrupt;
    }

#if defined(QELFPARSER_DEBUG)
    qDebug(".shstrtab at 0x%s", QByteArray::number(m_stringTableFileOffset, 16).data());
#endif

    const char *s = dataStart + e_shoff;
    for (int i = 0; i < e_shnum; ++i) {
        ElfSectionHeader sh;
        parseSectionHeader(s, &sh);
        if (sh.name == 0) {
            s += e_shentsize;
            continue;
        }
        const char *shnam = dataStart + m_stringTableFileOffset + sh.name;

        if (m_stringTableFileOffset + sh.name > fdlen) {
            if (lib)
                lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                    .arg(library, QLibrary::tr("section name %1 of %2 behind end of file")
                                  .arg(i).arg(e_shnum));
            return Corrupt;
        }

#if defined(QELFPARSER_DEBUG)
        qDebug() << "++++" << i << shnam;
#endif

        if (qstrcmp(shnam, ".qtmetadata") == 0 || qstrcmp(shnam, ".rodata") == 0) {
            if (!(sh.type & 0x1)) {
                if (shnam[1] == 'r') {
                    if (lib)
                        lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                            .arg(library, QLibrary::tr("empty .rodata. not a library."));
                    return Corrupt;
                }
#if defined(QELFPARSER_DEBUG)
                qDebug()<<"section is not program data. skipped.";
#endif
                s += e_shentsize;
                continue;
            }

            if (sh.offset == 0 || (sh.offset + sh.size) > fdlen || sh.size < 1) {
                if (lib)
                    lib->errorString = QLibrary::tr("'%1' is an invalid ELF object (%2)")
                        .arg(library, QLibrary::tr("missing section data. This is not a library."));
                return Corrupt;
            }
            *pos = sh.offset;
            *sectionlen = sh.size;
            if (shnam[1] == 'q')
                return QtMetaDataSection;
        }
        s += e_shentsize;
    }
    return NoQtSection;
}

QT_END_NAMESPACE

#endif // defined(Q_OF_ELF) && defined(Q_CC_GNU)
