// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstorageinfo_linux_p.h"

#include "qdiriterator.h"
#include <private/qcore_unix_p.h>

#if defined(Q_OS_ANDROID)
#  include <sys/mount.h>
#  include <sys/vfs.h>
#  define QT_STATFS    ::statfs
#  define QT_STATFSBUF struct statfs
#  if !defined(ST_RDONLY)
#    define ST_RDONLY 1 // hack for missing define on Android
#  endif
#else
#  include <sys/statvfs.h>
#  if defined(QT_LARGEFILE_SUPPORT)
#    define QT_STATFSBUF struct statvfs64
#    define QT_STATFS    ::statvfs64
#  else
#    define QT_STATFSBUF struct statvfs
#    define QT_STATFS    ::statvfs
#  endif // QT_LARGEFILE_SUPPORT
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// udev encodes the labels with ID_LABEL_FS_ENC which is done with
// blkid_encode_string(). Within this function some 1-byte utf-8
// characters not considered safe (e.g. '\' or ' ') are encoded as hex
static QString decodeFsEncString(const QString &str)
{
    QString decoded;
    decoded.reserve(str.size());

    int i = 0;
    while (i < str.size()) {
        if (i <= str.size() - 4) {    // we need at least four characters \xAB
            if (QStringView{str}.sliced(i).startsWith("\\x"_L1)) {
                bool bOk;
                const int code = QStringView{str}.mid(i+2, 2).toInt(&bOk, 16);
                if (bOk && code >= 0x20 && code < 0x80) {
                    decoded += QChar(code);
                    i += 4;
                    continue;
                }
            }
        }
        decoded += str.at(i);
        ++i;
    }
    return decoded;
}

static inline dev_t deviceIdForPath(const QString &device)
{
    QT_STATBUF st;
    if (QT_STAT(QFile::encodeName(device), &st) < 0)
        return 0;
    return st.st_dev;
}

static inline quint64 retrieveDeviceId(const QByteArray &device, quint64 deviceId = 0)
{
    // major = 0 implies an anonymous block device, so we need to stat() the
    // actual device to get its dev_t. This is required for btrfs (and possibly
    // others), which always uses them for all the subvolumes (including the
    // root):
    // https://codebrowser.dev/linux/linux/fs/btrfs/disk-io.c.html#btrfs_init_fs_root
    // https://codebrowser.dev/linux/linux/fs/super.c.html#get_anon_bdev
    // For everything else, we trust the parameter.
    if (major(deviceId) != 0)
        return deviceId;

    // don't even try to stat() a relative path or "/"
    if (device.size() < 2 || !device.startsWith('/'))
        return 0;

    QT_STATBUF st;
    if (QT_STAT(device, &st) < 0)
        return 0;
    if (!S_ISBLK(st.st_mode))
        return 0;
    return st.st_rdev;
}

static inline QString retrieveLabel(const QByteArray &device, quint64 deviceId)
{
    static const char pathDiskByLabel[] = "/dev/disk/by-label";

    deviceId = retrieveDeviceId(device, deviceId);
    if (!deviceId)
        return QString();

    auto filter = QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot;
    QDirIterator it(QLatin1StringView(pathDiskByLabel), filter);
    while (it.hasNext()) {
        QFileInfo fileInfo = it.nextFileInfo();
        QString name = fileInfo.fileName();
        if (retrieveDeviceId(QFile::encodeName(fileInfo.filePath())) == deviceId)
            return decodeFsEncString(std::move(name));
    }
    return QString();
}

void QStorageInfoPrivate::doStat()
{
    quint64 deviceId = initRootPath();
    if (!deviceId)
        return;

    retrieveVolumeInfo();
    name = retrieveLabel(device, deviceId);
}

void QStorageInfoPrivate::retrieveVolumeInfo()
{
    QT_STATFSBUF statfs_buf;
    int result;
    EINTR_LOOP(result, QT_STATFS(QFile::encodeName(rootPath).constData(), &statfs_buf));
    if (result == 0) {
        valid = true;
        ready = true;

        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
        blockSize = statfs_buf.f_bsize;

#if defined(Q_OS_ANDROID)
#if defined(_STATFS_F_FLAGS)
        readOnly = (statfs_buf.f_flags & ST_RDONLY) != 0;
#endif
#else
        readOnly = (statfs_buf.f_flag & ST_RDONLY) != 0;
#endif
    }
}

static std::vector<MountInfo> parseMountInfo(FilterMountInfo filter = FilterMountInfo::All)
{
    QFile file(u"/proc/self/mountinfo"_s);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QByteArray mountinfo = file.readAll();
    file.close();

    return doParseMountInfo(mountinfo, filter);
}

quint64 QStorageInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();
    if (rootPath.isEmpty())
        return 0;

    std::vector<MountInfo> infos = parseMountInfo();
    if (infos.empty()) {
        rootPath = u'/';

        // Need to return something non-zero here for this unlikely condition.
        // Linux currently uses 20 bits for the minor portion[1] in a 32-bit
        // integer; glibc, MUSL, and 64-bit Bionic use a 64-bit userspace
        // dev_t, so this value will not match a real device from the kernel.
        // 32-bit Bionic still has a 32-bit dev_t, but its makedev() macro[2]
        // returns 64-bit content too.
        // [1] https://codebrowser.dev/linux/linux/include/linux/kdev_t.h.html#_M/MINORBITS
        // [2] https://android.googlesource.com/platform/bionic/+/ndk-r19/libc/include/sys/sysmacros.h#39
        return makedev(0, -1);
    }

    // We iterate over the /proc/self/mountinfo list backwards because then any
    // matching isParentOf must be the actual mount point because it's the most
    // recent mount on that path. Linux does allow mounting over non-empty
    // directories, such as in:
    //   # mount | tail -2
    //   tmpfs on /tmp/foo/bar type tmpfs (rw,relatime,inode64)
    //   tmpfs on /tmp/foo type tmpfs (rw,relatime,inode64)
    // But just in case there's a mount --move, we ensure the device ID does
    // match.
    const QString oldRootPath = std::exchange(rootPath, QString());
    const dev_t rootPathDevId = deviceIdForPath(oldRootPath);
    for (auto it = infos.rbegin(); it != infos.rend(); ++it) {
        if (rootPathDevId != it->stDev || !isParentOf(it->mountPoint, oldRootPath))
            continue;
        rootPath = std::move(it->mountPoint);
        device = std::move(it->device);
        fileSystemType = std::move(it->fsType);
        subvolume = std::move(it->fsRoot);
        return it->stDev;
    }
    return 0;
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    std::vector<MountInfo> infos = parseMountInfo(FilterMountInfo::Filtered);
    if (infos.empty())
        return QList{root()};

    QList<QStorageInfo> volumes;
    for (MountInfo &info : infos) {
        QStorageInfoPrivate d(std::move(info));
        d.retrieveVolumeInfo();
        if (d.bytesTotal == 0 && d.rootPath != u'/')
            continue;
        if (info.stDev != deviceIdForPath(d.rootPath))
            continue;       // probably something mounted over this mountpoint
        d.name = retrieveLabel(d.device, info.stDev);
        volumes.emplace_back(QStorageInfo(*new QStorageInfoPrivate(std::move(d))));
    }
    return volumes;
}

QT_END_NAMESPACE
