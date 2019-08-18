/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timeline.h"

#include <QDebug>

#include "node/block/gap/gap.h"
#include "node/graph.h"

TimelineOutput::TimelineOutput() :
  attached_timeline_(nullptr)
{
  track_input_ = new NodeInput("track_in");
  track_input_->add_data_input(NodeParam::kTrack);
  AddParameter(track_input_);

  connect(this, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(TrackConnectionAdded(NodeEdgePtr)));
  connect(this, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(TrackConnectionRemoved(NodeEdgePtr)));
}

QString TimelineOutput::Name()
{
  return tr("Timeline");
}

QString TimelineOutput::id()
{
  return "org.olivevideoeditor.Olive.timeline";
}

QString TimelineOutput::Category()
{
  return tr("Output");
}

QString TimelineOutput::Description()
{
  return tr("Node for communicating between a Timeline panel and the node graph.");
}

void TimelineOutput::AttachTimeline(TimelinePanel *timeline)
{
  if (attached_timeline_ != nullptr) {
    //TimelineView* view = attached_timeline_->view();

    //disconnect(view, SIGNAL(RequestInsertBlockAtIndex(Block*, int)), this, SLOT(InsertBlockAtIndex(Block*, int)));
    //disconnect(view, SIGNAL(RequestPlaceBlock(Block*, rational)), this, SLOT(PlaceBlock(Block*, rational)));

    // Remove existing UI objects from TimelinePanel
    attached_timeline_->Clear();
  }

  attached_timeline_ = timeline;

  if (attached_timeline_ != nullptr) {
    // FIXME: TEST CODE ONLY
    attached_timeline_->SetTimebase(rational(1001, 30000));
    // END TEST CODE

    foreach (TrackOutput* track, track_cache_) {
      // Defer to the track to make all the block UI items necessary
      track->GenerateBlockWidgets();
    }

    //TimelineView* view = attached_timeline_->view();

    //connect(view, SIGNAL(RequestInsertBlockAtIndex(Block*, int)), this, SLOT(InsertBlockAtIndex(Block*, int)));
    //connect(view, SIGNAL(RequestPlaceBlock(Block*, rational)), this, SLOT(PlaceBlock(Block*, rational)));
  }
}

NodeInput *TimelineOutput::track_input()
{
  return track_input_;
}

void TimelineOutput::Process(const rational &time)
{
  Q_UNUSED(time)
}

int TimelineOutput::GetTrackIndex(TrackOutput *track)
{
  return track_cache_.indexOf(track);
}

rational TimelineOutput::GetSequenceLength()
{
  rational length = 0;

  foreach (TrackOutput* track, track_cache_) {
    length = qMax(length, track->in());
  }

  return length;
}

TrackOutput *TimelineOutput::attached_track()
{
  return ValueToPtr<TrackOutput>(track_input_->get_value(0));
}

void TimelineOutput::AttachTrack(TrackOutput *track)
{
  TrackOutput* current_track = track;

  // Traverse through Tracks caching and connecting them
  while (current_track != nullptr) {
    connect(current_track, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(TrackEdgeAdded(NodeEdgePtr)));
    connect(current_track, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(TrackEdgeRemoved(NodeEdgePtr)));
    connect(current_track, SIGNAL(BlockAdded(Block*)), this, SLOT(TrackAddedBlock(Block*)));
    connect(current_track, SIGNAL(BlockRemoved(Block*)), this, SLOT(TrackRemovedBlock(Block*)));

    track_cache_.append(current_track);

    // This function must be called after the track is added to track_cache_, since it uses track_cache_ to determine
    // the track's index
    current_track->GenerateBlockWidgets();

    current_track = current_track->next_track();
  }
}

void TimelineOutput::DetachTrack(TrackOutput *track)
{
  TrackOutput* current_track = track;

  // Traverse through Tracks uncaching and disconnecting them
  while (current_track != nullptr) {
    current_track->DestroyBlockWidgets();

    disconnect(current_track, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(TrackEdgeAdded(NodeEdgePtr)));
    disconnect(current_track, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(TrackEdgeRemoved(NodeEdgePtr)));
    disconnect(current_track, SIGNAL(BlockAdded(Block*)), this, SLOT(TrackAddedBlock(Block*)));
    disconnect(current_track, SIGNAL(BlockRemoved(Block*)), this, SLOT(TrackRemovedBlock(Block*)));

    track_cache_.removeAll(current_track);

    current_track = current_track->next_track();
  }
}

void TimelineOutput::TrackConnectionAdded(NodeEdgePtr edge)
{
  if (edge->input() != track_input()) {
    return;
  }

  AttachTrack(attached_track());

  // FIXME: TEST CODE ONLY
  if (attached_timeline_ != nullptr) {
    attached_timeline_->SetTimebase(rational(1001, 30000));
  }
  // END TEST CODE
}

void TimelineOutput::TrackConnectionRemoved(NodeEdgePtr edge)
{
  if (edge->input() != track_input()) {
    return;
  }

  DetachTrack(ValueToPtr<TrackOutput>(edge->output()->get_value(0)));

  if (attached_timeline_ != nullptr) {
    attached_timeline_->Clear();
  }
}

void TimelineOutput::TrackAddedBlock(Block *block)
{
  if (attached_timeline_ != nullptr) {
    qDebug() << "track index lmao:" << GetTrackIndex(static_cast<TrackOutput*>(sender()));

    attached_timeline_->view()->AddBlock(block);
  }
}

void TimelineOutput::TrackRemovedBlock(Block *block)
{
  if (attached_timeline_ != nullptr) {
    attached_timeline_->view()->RemoveBlock(block);
  }
}

void TimelineOutput::TrackEdgeAdded(NodeEdgePtr edge)
{
  // Assume this signal was sent from a TrackOutput
  TrackOutput* track = static_cast<TrackOutput*>(sender());

  // If this edge pertains to the track's track input, all the tracks just added need attaching
  if (edge->input() == track->track_input()) {
    TrackOutput* added_track = ValueToPtr<TrackOutput>(edge->output()->get_value(0));

    AttachTrack(added_track);
  }
}

void TimelineOutput::TrackEdgeRemoved(NodeEdgePtr edge)
{
  // Assume this signal was sent from a TrackOutput
  TrackOutput* track = static_cast<TrackOutput*>(sender());

  // If this edge pertains to the track's track input, all the tracks just added need attaching
  if (edge->input() == track->track_input()) {
    TrackOutput* added_track = ValueToPtr<TrackOutput>(edge->output()->get_value(0));

    DetachTrack(added_track);
  }
}
