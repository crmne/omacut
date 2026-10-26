#pragma once

#include <QList>
#include <QString>

// What gets exported: an ordered list of clips, each a stretch of one of the
// loaded videos, played back to back. The videos are never modified; a clip's
// in and out are seconds in its source.
namespace edit {

struct Clip {
    int source = 0;
    double in = 0.0;
    double out = 0.0;
    double length() const { return out - in; }
    bool operator==(const Clip &other) const {
        return source == other.source && in == other.in && out == other.out;
    }
};
using Clips = QList<Clip>;

// A recording laid over the edit from `at` seconds along it, heard on top
// of whatever the clips sound like there.
struct Voiceover {
    QString path;
    double at = 0.0;
    double length = 0.0;
    double end() const { return at + length; }
    bool operator==(const Voiceover &other) const {
        return path == other.path && at == other.at && length == other.length;
    }
};
using Voiceovers = QList<Voiceover>;

constexpr double minimumClip = 0.1;

double duration(const Clips &clips);
// Neighbours that carry on where the one before left off in the same source,
// merged: what an export encodes.
Clips merged(const Clips &clips);
// Nothing cut, added or reordered: all of the first source, as it is.
bool untouched(const Clips &clips, double firstSourceDuration);

}  // namespace edit
