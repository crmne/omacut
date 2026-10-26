#pragma once

#include <QObject>
#include <QVariantList>

#include "edit.h"

// The clips being edited, and the voiceovers laid over them, with undo. A gesture (a drag) is one undo step.
// Times called "t" are seconds along the edited sequence; a clip's in and out
// are seconds in its source. Shared by the real backend and the QML tests'
// stand-in, so both edit the same way.
class Timeline : public QObject {
    Q_OBJECT
    // Each clip as {source, in, out, start, end}, start and end along the sequence.
    Q_PROPERTY(QVariantList clips READ clipList NOTIFY changed)
    Q_PROPERTY(double duration READ duration NOTIFY changed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    // Cut, added to, reordered or voiced over, and not exported as it stands.
    Q_PROPERTY(bool unexported READ unexported NOTIFY changed)
    // Each voiceover as {url, at, end, length}, at and end along the sequence.
    // Only notified when they change, so their players aren't rebuilt by
    // every trim.
    Q_PROPERTY(QVariantList voiceovers READ voiceoverList NOTIFY voiceoversChanged)

public:
    explicit Timeline(QObject *parent = nullptr) : QObject(parent) {}

    edit::Clips clips() const { return m_now.clips; }
    QVariantList clipList() const;
    edit::Voiceovers voiceovers() const { return m_now.voiceovers; }
    QVariantList voiceoverList() const;
    double duration() const { return edit::duration(m_now.clips); }
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    bool unexported() const;

    // A freshly loaded video: one clip spanning all of it, no voiceovers,
    // and no history.
    void reset(double duration);
    // Adds a video of the given length as a clip after the one under t (at
    // the end past the last), as an undoable edit. Returns its source index.
    int addSource(double duration, double t);
    void markExported(const edit::Clips &clips, const edit::Voiceovers &voiceovers = {});
    // Lays a recording of the given length over the sequence from at, as an
    // undoable edit.
    void addVoiceover(const QString &path, double at, double length);

    // The clip under t; the end of the sequence belongs to the last clip.
    Q_INVOKABLE int clipAt(double t) const;
    Q_INVOKABLE double clipStart(int index) const;
    // The nearest clip edge before (direction < 0) or after t, else the
    // sequence's start or end.
    Q_INVOKABLE double edgeFrom(double t, int direction) const;
    // Whether clip index and the next can join: the next carries on where
    // index leaves off, in the same source.
    Q_INVOKABLE bool canJoin(int index) const;

    Q_INVOKABLE void split(double t);
    // Clamps into the clip's source, keeping at least a minimum clip.
    Q_INVOKABLE void setClip(int index, double in, double out);
    // Moves one edge of a clip to a source time, stopping a minimum clip short
    // of the other edge.
    Q_INVOKABLE void moveEdge(int index, bool start, double sourceTime);
    // Moves the start or end of the clip under t to t.
    Q_INVOKABLE void trimTo(double t, bool start);
    Q_INVOKABLE void removeClip(int index);
    Q_INVOKABLE void removeAt(double t);
    // Moves clip from to position to, shifting the clips in between.
    Q_INVOKABLE void moveClip(int from, int to);
    Q_INVOKABLE void joinClips(int index);
    // Moves a voiceover to start at t, kept from starting before the
    // sequence or past its end.
    Q_INVOKABLE void moveVoiceover(int index, double t);
    Q_INVOKABLE void removeVoiceover(int index);
    Q_INVOKABLE void beginGesture();
    Q_INVOKABLE void endGesture();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

signals:
    void changed();
    void voiceoversChanged();

private:
    // What an undo step holds.
    struct State {
        edit::Clips clips;
        edit::Voiceovers voiceovers;
        bool operator==(const State &other) const {
            return clips == other.clips && voiceovers == other.voiceovers;
        }
    };
    void apply(const edit::Clips &next);
    void apply(const State &next);
    // Replaces the current state, telling whoever watches what changed.
    void setState(const State &next);
    double voiceoverStart(double t) const;

    QList<double> m_sourceDurations;
    State m_now;
    State m_exported;
    QList<State> m_undo;
    QList<State> m_redo;
    bool m_gesture = false;
    bool m_gestureRecorded = false;
};
