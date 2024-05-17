#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Parse CLDR data for QTimeZone use with MS-Windows

Script to parse the CLDR common/supplemental/windowsZones.xml file and
prepare its data for use in QTimeZone.  See ``./cldr2qlocalexml.py`` for
where to get the CLDR data.  Pass its root directory as first parameter
to this script.  You can optionally pass the qtbase root directory as
second parameter; it defaults to the root of the checkout containing
this script.  This script updates qtbase's
src/corelib/time/qtimezoneprivate_data_p.h with the new data.
"""

import datetime
from pathlib import Path
import textwrap
import argparse

from localetools import unicode2hex, wrap_list, Error, SourceFileEditor, qtbase_root
from cldr import CldrAccess
# This script shall report any updates zonedata may need.
from zonedata import windowsIdList, utcIdList

class ByteArrayData:
    def __init__(self):
        self.data = []
        self.hash = {}

    def append(self, s):
        s = s + '\0'
        if s in self.hash:
            return self.hash[s]

        lst = unicode2hex(s)
        index = len(self.data)
        if index > 0xffff:
            raise Error(f'Index ({index}) outside the uint16 range !')
        self.hash[s] = index
        self.data += lst
        return index

    def write(self, out, name):
        out(f'\nstatic constexpr char {name}[] = {{\n')
        out(wrap_list(self.data, 16)) # 16 == 100 // len('0xhh, ')
        # Will over-spill 100-col if some 4-digit hex show up, but none do (yet).
        out('\n};\n')

class ZoneIdWriter (SourceFileEditor):
    # All the output goes into namespace QtTimeZoneCldr.
    def write(self, version, defaults, windowsIds):
        self.__writeWarning(version)
        windows, iana = self.__writeTables(self.writer.write, defaults, windowsIds)
        windows.write(self.writer.write, 'windowsIdData')
        iana.write(self.writer.write, 'ianaIdData')

    def __writeWarning(self, version):
        self.writer.write(f"""
/*
    This part of the file was generated on {datetime.date.today()} from the
    Common Locale Data Repository v{version} file supplemental/windowsZones.xml

    http://www.unicode.org/cldr/

    Do not edit this code: run cldr2qtimezone.py on updated (or
    edited) CLDR data; see qtbase/util/locale_database/.
*/

""")

    @staticmethod
    def __writeTables(out, defaults, windowsIds):
        windowsIdData, ianaIdData = ByteArrayData(), ByteArrayData()

        # Write Windows/IANA table
        out('// Windows ID Key, Territory Enum, IANA ID Index\n')
        out('static constexpr ZoneData zoneDataTable[] = {\n')
        # Sorted by (Windows ID Key, territory enum)
        for index, data in sorted(windowsIds.items()):
            out('    {{ {:6d},{:6d},{:6d} }}, // {} / {}\n'.format(
                    data['windowsKey'], data['territoryId'],
                    ianaIdData.append(data['ianaList']),
                    data['windowsId'], data['territory']))
        out('};\n\n')

        # Write Windows ID key table
        out('// Windows ID Key, Windows ID Index, IANA ID Index, UTC Offset\n')
        out('static constexpr WindowsData windowsDataTable[] = {\n')
        # Sorted by Windows ID key; sorting case-insensitively by
        # Windows ID must give the same order.
        winIdNames = [x.lower() for x, y in windowsIdList]
        assert all(x == y for x, y in zip(winIdNames, sorted(winIdNames))), \
            [(x, y) for x, y in zip(winIdNames, sorted(winIdNames)) if x != y]
        for index, pair in enumerate(windowsIdList, 1):
            out('    {{ {:6d},{:6d},{:6d},{:6d} }}, // {}\n'.format(
                    index,
                    windowsIdData.append(pair[0]),
                    ianaIdData.append(defaults[index]),
                    pair[1], pair[0]))
        out('};\n\n')

        offsetMap = {}
        for pair in utcIdList:
            offsetMap[pair[1]] = offsetMap.get(pair[1], ()) + (pair[0],)
        # Write UTC ID key table
        out('// IANA ID Index, UTC Offset\n')
        out('static constexpr UtcData utcDataTable[] = {\n')
        for offset in sorted(offsetMap.keys()): # Sort so C++ can binary-chop.
            names = offsetMap[offset];
            out('    {{ {:6d},{:6d} }}, // {}\n'.format(
                    ianaIdData.append(' '.join(names)), offset, names[0]))
        out('};\n')

        return windowsIdData, ianaIdData


def main(out, err):
    """Parses CLDR's data and updates Qt's representation of it.

    Takes sys.stdout, sys.stderr (or equivalents) as
    arguments. Expects two command-line options: the root of the
    unpacked CLDR data-file tree and the root of the qtbase module's
    checkout. Updates QTimeZone's private data about Windows time-zone
    IDs."""
    parser = argparse.ArgumentParser(
        description="Update Qt's CLDR-derived timezone data.")
    parser.add_argument('cldr_path', help='path to the root of the CLDR tree')
    parser.add_argument('qtbase_path',
                        help='path to the root of the qtbase source tree',
                        nargs='?', default=qtbase_root)

    args = parser.parse_args()

    cldrPath = Path(args.cldr_path)
    qtPath = Path(args.qtbase_path)

    if not qtPath.is_dir():
        parser.error(f"No such Qt directory: {qtPath}")

    if not cldrPath.is_dir():
        parser.error(f"No such CLDR directory: {cldrPath}")

    dataFilePath = qtPath.joinpath('src/corelib/time/qtimezoneprivate_data_p.h')

    if not dataFilePath.is_file():
        parser.error(f'No such file: {dataFilePath}')

    try:
        version, defaults, winIds = CldrAccess(cldrPath).readWindowsTimeZones(
            dict((name, ind) for ind, name in enumerate((x[0] for x in windowsIdList), 1)))
    except IOError as e:
        parser.error(
            f'Failed to open common/supplemental/windowsZones.xml: {e}')
        return 1
    except Error as e:
        err.write('\n'.join(textwrap.wrap(
                    f'Failed to read windowsZones.xml: {e}',
                    subsequent_indent=' ', width=80)) + '\n')
        return 1

    out.write('Input file parsed, now writing data\n')

    try:
        with ZoneIdWriter(dataFilePath, qtPath) as writer:
            writer.write(version, defaults, winIds)
    except Exception as e:
        err.write(f'\nError while updating timezone data: {e}\n')
        return 1

    out.write(f'Data generation completed, please check the new file at {dataFilePath}\n')
    return 0

if __name__ == '__main__':
    import sys
    sys.exit(main(sys.stdout, sys.stderr))
