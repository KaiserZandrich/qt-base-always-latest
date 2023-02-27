// Copyright (C) 2016 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "dnslookup.h"

#include <QCoreApplication>
#include <QDnsLookup>
#include <QHostAddress>
#include <QStringList>
#include <QTimer>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <stdio.h>

static int typeFromParameter(QStringView type)
{
    if (type.compare(u"a", Qt::CaseInsensitive) == 0)
        return QDnsLookup::A;
    if (type.compare(u"aaaa", Qt::CaseInsensitive) == 0)
        return QDnsLookup::AAAA;
    if (type.compare(u"any", Qt::CaseInsensitive) == 0)
        return QDnsLookup::ANY;
    if (type.compare(u"cname", Qt::CaseInsensitive) == 0)
        return QDnsLookup::CNAME;
    if (type.compare(u"mx", Qt::CaseInsensitive) == 0)
        return QDnsLookup::MX;
    if (type.compare(u"ns", Qt::CaseInsensitive) == 0)
        return QDnsLookup::NS;
    if (type.compare(u"ptr", Qt::CaseInsensitive) == 0)
        return QDnsLookup::PTR;
    if (type.compare(u"srv", Qt::CaseInsensitive) == 0)
        return QDnsLookup::SRV;
    if (type.compare(u"txt", Qt::CaseInsensitive) == 0)
        return QDnsLookup::TXT;
    return -1;
}

//! [0]

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, DnsQuery *query, QString *errorMessage)
{
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption nameServerOption("n", "The name server to use.", "nameserver");
    parser.addOption(nameServerOption);
    const QCommandLineOption typeOption("t", "The lookup type.", "type");
    parser.addOption(typeOption);
    parser.addPositionalArgument("name", "The name to look up.");
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments())) {
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    if (parser.isSet(nameServerOption)) {
        const QString nameserver = parser.value(nameServerOption);
        query->nameServer = QHostAddress(nameserver);
        if (query->nameServer.isNull() || query->nameServer.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol) {
            *errorMessage = "Bad nameserver address: " + nameserver;
            return CommandLineError;
        }
    }

    if (parser.isSet(typeOption)) {
        const QString typeParameter = parser.value(typeOption);
        const int type = typeFromParameter(typeParameter);
        if (type < 0) {
            *errorMessage = "Bad record type: " + typeParameter;
            return CommandLineError;
        }
        query->type = static_cast<QDnsLookup::Type>(type);
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        *errorMessage = "Argument 'name' missing.";
        return CommandLineError;
    }
    if (positionalArguments.size() > 1) {
        *errorMessage = "Several 'name' arguments specified.";
        return CommandLineError;
    }
    query->name = positionalArguments.first();

    return CommandLineOk;
}

//! [0]

DnsManager::DnsManager()
    : dns(new QDnsLookup(this))
{
    connect(dns, &QDnsLookup::finished, this, &DnsManager::showResults);
}

void DnsManager::execute()
{
    // lookup type
    dns->setType(query.type);
    if (!query.nameServer.isNull())
        dns->setNameserver(query.nameServer);
    dns->setName(query.name);
    dns->lookup();
}

void DnsManager::showResults()
{
    if (dns->error() != QDnsLookup::NoError)
        printf("Error: %i (%s)\n", dns->error(), qPrintable(dns->errorString()));

    // CNAME records
    const QList<QDnsDomainNameRecord> cnameRecords = dns->canonicalNameRecords();
    for (const QDnsDomainNameRecord &record : cnameRecords) {
        printf("%s\t%i\tIN\tCNAME\t%s\n", qPrintable(record.name()), record.timeToLive(),
               qPrintable(record.value()));
    }

    // A and AAAA records
    const QList<QDnsHostAddressRecord> aRecords = dns->hostAddressRecords();
    for (const QDnsHostAddressRecord &record : aRecords) {
        const char *type =
                (record.value().protocol() == QAbstractSocket::IPv6Protocol) ? "AAAA" : "A";
        printf("%s\t%i\tIN\t%s\t%s\n", qPrintable(record.name()), record.timeToLive(), type,
               qPrintable(record.value().toString()));
    }

    // MX records
    const QList<QDnsMailExchangeRecord> mxRecords = dns->mailExchangeRecords();
    for (const QDnsMailExchangeRecord &record : mxRecords) {
        printf("%s\t%i\tIN\tMX\t%u %s\n", qPrintable(record.name()), record.timeToLive(),
               record.preference(), qPrintable(record.exchange()));
    }

    // NS records
    const QList<QDnsDomainNameRecord> nsRecords = dns->nameServerRecords();
    for (const QDnsDomainNameRecord &record : nsRecords) {
        printf("%s\t%i\tIN\tNS\t%s\n", qPrintable(record.name()), record.timeToLive(),
               qPrintable(record.value()));
    }

    // PTR records
    const QList<QDnsDomainNameRecord> ptrRecords = dns->pointerRecords();
    for (const QDnsDomainNameRecord &record : ptrRecords) {
        printf("%s\t%i\tIN\tPTR\t%s\n", qPrintable(record.name()), record.timeToLive(),
               qPrintable(record.value()));
    }

    // SRV records
    const QList<QDnsServiceRecord> srvRecords = dns->serviceRecords();
    for (const QDnsServiceRecord &record : srvRecords) {
        printf("%s\t%i\tIN\tSRV\t%u %u %u %s\n", qPrintable(record.name()), record.timeToLive(),
               record.priority(), record.weight(), record.port(), qPrintable(record.target()));
    }

    // TXT records
    const QList<QDnsTextRecord> txtRecords = dns->textRecords();
    for (const QDnsTextRecord &record : txtRecords) {
        QStringList values;
        const QList<QByteArray> dnsRecords = record.values();
        for (const QByteArray &ba : dnsRecords)
            values << "\"" + QString::fromLatin1(ba) + "\"";
        printf("%s\t%i\tIN\tTXT\t%s\n", qPrintable(record.name()), record.timeToLive(),
               qPrintable(values.join(' ')));
    }

    QCoreApplication::instance()->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//! [1]
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCoreApplication::setApplicationName(QCoreApplication::translate("QDnsLookupExample",
                                                                     "DNS Lookup Example"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("QDnsLookupExample",
                                                                 "An example demonstrating the "
                                                                 "class QDnsLookup."));
    DnsQuery query;
    QString errorMessage;
    switch (parseCommandLine(parser, &query, &errorMessage)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        fputs(qPrintable(errorMessage), stderr);
        fputs("\n\n", stderr);
        fputs(qPrintable(parser.helpText()), stderr);
        return 1;
    case CommandLineVersionRequested:
        parser.showVersion();
        Q_UNREACHABLE_RETURN(0);
    case CommandLineHelpRequested:
        parser.showHelp();
        Q_UNREACHABLE_RETURN(0);
    }
//! [1]

    DnsManager manager;
    manager.setQuery(query);
    QTimer::singleShot(0, &manager, SLOT(execute()));

    return app.exec();
}
