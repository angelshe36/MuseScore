//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: keyb.cpp 5658 2012-05-21 18:40:58Z wschweer $
//
//  Copyright (C) 2002-2011 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "scoreview.h"
#include "libmscore/note.h"
#include "libmscore/sym.h"
#include "libmscore/note.h"
#include "libmscore/score.h"
#include "libmscore/rest.h"
#include "libmscore/chord.h"
#include "libmscore/select.h"
#include "libmscore/input.h"
#include "libmscore/key.h"
#include "libmscore/measure.h"
#include "musescore.h"
#include "libmscore/slur.h"
#include "libmscore/tuplet.h"
#include "libmscore/text.h"
#include "libmscore/staff.h"
#include "libmscore/part.h"
#include "drumtools.h"
#include "preferences.h"
#include "libmscore/segment.h"
#include "libmscore/mscore.h"
#include "libmscore/stafftype.h"
#include "texttools.h"

namespace Ms {

#ifdef Q_OS_MAC
#define CONTROL_MODIFIER Qt::AltModifier
#else
#define CONTROL_MODIFIER Qt::ControlModifier
#endif

//---------------------------------------------------------
//   Canvas::editCmd
//---------------------------------------------------------

void ScoreView::editCmd(const QString& cmd)
      {
      if (!editElement)
            return;

      if (editElement->isLyrics()) {
            if (cmd == "next-lyric")
                  lyricsTab(false, true, false);
            else if (cmd == "prev-lyric")
                  lyricsTab(true, true, false);
            }
      }

//---------------------------------------------------------
//   editKeyLyrics
//---------------------------------------------------------

bool ScoreView::editKeyLyrics(QKeyEvent* ev)
      {
      editData.key       = ev->key();
      editData.modifiers = ev->modifiers();
      editData.s         = ev->text();
      bool ctrl          = editData.modifiers == Qt::ControlModifier;

      switch (editData.key) {
            case Qt::Key_Space:
                  if (!(editData.modifiers & CONTROL_MODIFIER)) {
                        if (editData.s == "_")
                              lyricsUnderscore();
                        else // TODO: shift+tab events are filtered by qt
                              lyricsTab(editData.modifiers & Qt::ShiftModifier, true, false);
                        }
                  else
                        return false;
                  break;

            case Qt::Key_Left:
                  if (!ctrl && editElement->edit(editData)) {
                        mscore->textTools()->updateTools(editData);
                        _score->update();
                        mscore->endCmd();
                        }
                  else
                        lyricsTab(true, true, true);      // go to previous lyrics
                  break;
            case Qt::Key_Right:
                  if (!ctrl && editElement->edit(editData)) {
                        mscore->textTools()->updateTools(editData);
                        _score->update();
                        mscore->endCmd();
                        }
                  else
                        lyricsTab(false, false, true);    // go to next lyrics
                  break;
            case Qt::Key_Up:
                  lyricsUpDown(true, true);
                  break;
            case Qt::Key_Down:
                  lyricsUpDown(false, true);
                  break;
            case Qt::Key_Return:
                  lyricsReturn();
                  break;
            default:
                  {
                  if (editData.s == "-" && !(editData.modifiers & CONTROL_MODIFIER))
                        lyricsMinus();
                  else if (editData.s == "_" && !(editData.modifiers & CONTROL_MODIFIER))
                        lyricsUnderscore();
                  else
                        return false;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   editKey
//---------------------------------------------------------

void ScoreView::editKey(QKeyEvent* ev)
      {
      if (!editElement)
            return;
      if (ev->type() == QEvent::KeyRelease) {
            auto modifiers = Qt::ControlModifier | Qt::ShiftModifier;
            if (editElement->isText() && ((ev->modifiers() & modifiers) == 0)) {
                  Text* text = toText(editElement);
                  text->endHexState();
                  ev->accept();
                  update();
                  }
            return;
            }
      editData.view      = this;
      editData.key       = ev->key();
      editData.modifiers = ev->modifiers();
      editData.s         = ev->text();

      if (MScore::debugMode)
            qDebug("keyPressEvent key 0x%02x(%c) mod 0x%04x <%s> nativeKey 0x%02x scancode %d",
               editData.key, editData.key, int(editData.modifiers), qPrintable(editData.s), ev->nativeVirtualKey(), ev->nativeScanCode());


      if (editElement->isLyrics()) {
            if (editKeyLyrics(ev)) {
                  ev->accept();
                  return;
                  }
            }
      else if (editElement->isHarmony()) {
/*
            if (editData.key == Qt::Key_Tab || editData.key == Qt::Key_Backtab) {
                  harmonyTab(editData.key == Qt::Key_Backtab ? true : (editData.modifiers & Qt::ShiftModifier));
                  ev->accept();
                  return;
                  }
*/
            if (editData.key == Qt::Key_Space && !(editData.modifiers & CONTROL_MODIFIER)) {
                  harmonyBeatsTab(true, editData.modifiers & Qt::ShiftModifier);
                  ev->accept();
                  return;
                  }
/*
            if (editData.key == Qt::Key_Semicolon || editData.key == Qt::Key_Colon) {
                  harmonyBeatsTab(false, editData.key == Qt::Key_Colon);
                  ev->accept();
                  return;
                  }
            if (editData.key >= Qt::Key_1 && editData.key <= Qt::Key_9 && (modifiers & CONTROL_MODIFIER)) {
                  int ticks = (MScore::division >> 4) << (editData.key - Qt::Key_1);
                  harmonyTicksTab(ticks);
                  ev->accept();
                  return;
                  }
*/
            }
      else if (editElement->type() == ElementType::FIGURED_BASS) {
            int found = false;
            if (editData.key == Qt::Key_Space && !(editData.modifiers & CONTROL_MODIFIER)) {
                  figuredBassTab(false, editData.modifiers & Qt::ShiftModifier);
                  found = true;
                  }
/*
            if (editData.key == Qt::Key_Tab || editData.key == Qt::Key_Backtab) {
                  figuredBassTab(true, editData.key == Qt::Key_Backtab ? true : (editData.modifiers & Qt::ShiftModifier) );
                  found = true;
                  }
            if (editData.key >= Qt::Key_1 && editData.key <= Qt::Key_9 && (editData.modifiers & CONTROL_MODIFIER)) {
                  int ticks = (MScore::division >> 4) << (editData.key - Qt::Key_1);
                  figuredBassTicksTab(ticks);
                  found = true;
                  }
*/
            if (found) {
                  ev->accept();
                  return;
                  }
            }


#ifdef Q_OS_WIN // Japenese IME on Windows needs to know when Contrl/Alt/Shift/CapsLock is pressed while in predit
      if (editElement->isText()) {
            Text* text = toText(editElement);
            if (text->cursor(editData)->format()->preedit() && QGuiApplication::inputMethod()->locale().script() == QLocale::JapaneseScript &&
                ((editData.key == Qt::Key_Control || (editData.modifiers & Qt::ControlModifier)) ||
                 (editData.key == Qt::Key_Alt     || (editData.modifiers & Qt::AltModifier)) ||
                 (editData.key == Qt::Key_Shift   || (editData.modifiers & Qt::ShiftModifier)) ||
                 (editData.key == Qt::Key_CapsLock))) {
                  return; // musescore will ignore this key event so that the IME can handle it
                  }
            }
#endif

      if (!((editData.modifiers & Qt::ShiftModifier) && (editData.key == Qt::Key_Backtab))) {
            _score->startCmd();
            if (editElement->edit(editData)) {
                  if (editElement->isText())
                        mscore->textTools()->updateTools(editData);
                  updateGrips();
                  ev->accept();
                  _score->endCmd();
                  mscore->endCmd();
                  return;
                  }
            if (editElement->isText() && (editData.key == Qt::Key_Left || editData.key == Qt::Key_Right)) {
                  ev->accept();
                  //return;
                  }
            _score->endCmd();
            mscore->endCmd();
            }
      QPointF delta;
      qreal _spatium = editElement->spatium();

      qreal xval, yval;
      if (editElement->isBeam()) {
            xval = 0.25 * _spatium;
            if (editData.modifiers & Qt::ControlModifier)
                  xval = _spatium;
            else if (editData.modifiers & Qt::AltModifier)
                  xval = 4 * _spatium;
            }
      else {
            xval = MScore::nudgeStep * _spatium;
            if (editData.modifiers & Qt::ControlModifier)
                  xval = MScore::nudgeStep10 * _spatium;
            else if (editData.modifiers & Qt::AltModifier)
                  xval = MScore::nudgeStep50 * _spatium;
            }
      yval = xval;

      if (mscore->vRaster()) {
            qreal vRaster = _spatium / MScore::vRaster();
            if (yval < vRaster)
                  yval = vRaster;
            }
      if (mscore->hRaster()) {
            qreal hRaster = _spatium / MScore::hRaster();
            if (xval < hRaster)
                  xval = hRaster;
            }
      // TODO: if raster, then xval/yval should be multiple of raster

      switch (editData.key) {
            case Qt::Key_Left:
                  delta = QPointF(-xval, 0);
                  break;
            case Qt::Key_Right:
                  delta = QPointF(xval, 0);
                  break;
            case Qt::Key_Up:
                  delta = QPointF(0, -yval);
                  break;
            case Qt::Key_Down:
                  delta = QPointF(0, yval);
                  break;
            default:
                  ev->ignore();
                  return;
            }
      editData.init();
      editData.delta   = delta;
      editData.view    = this;
      editData.hRaster = mscore->hRaster();
      editData.vRaster = mscore->vRaster();
      if (editData.curGrip != Grip::NO_GRIP && int(editData.curGrip) < editData.grips)
            editData.pos = editData.grip[int(editData.curGrip)].center() + delta;
      editElement->score()->startCmd();
      editElement->startEditDrag(editData);
      editElement->editDrag(editData);
      editElement->endEditDrag(editData);
      editElement->score()->endCmd();
      updateGrips();
      mscore->endCmd();
      ev->accept();
      }

//---------------------------------------------------------
//   updateInputState
//---------------------------------------------------------

void MuseScore::updateInputState(Score* score)
      {
      InputState& is = score->inputState();
      if (is.noteEntryMode()) {
            Staff* staff = score->staff(is.track() / VOICES);
            switch (staff->staffType(is.tick())->group()) {
                  case StaffGroup::STANDARD:
                        changeState(STATE_NOTE_ENTRY_STAFF_PITCHED);
                        break;
                  case StaffGroup::TAB:
                        changeState(STATE_NOTE_ENTRY_STAFF_TAB);
                        break;
                  case StaffGroup::PERCUSSION:
                        changeState(STATE_NOTE_ENTRY_STAFF_DRUM);
                        break;
                  }
            }

      getAction("pad-rest")->setChecked(is.rest());
      getAction("pad-dot")->setChecked(is.duration().dots() == 1);
      getAction("pad-dotdot")->setChecked(is.duration().dots() == 2);
      getAction("pad-dot3")->setChecked(is.duration().dots() == 3);
      getAction("pad-dot4")->setChecked(is.duration().dots() == 4);
      if ((mscore->state() & STATE_NORMAL) | (mscore->state() & STATE_NOTE_ENTRY)) {
            getAction("pad-dot")->setEnabled(true);
            getAction("pad-dotdot")->setEnabled(true);
            getAction("pad-dot3")->setEnabled(true);
            getAction("pad-dot4")->setEnabled(true);
            }
      switch (is.duration().type()) {
            case TDuration::DurationType::V_1024TH:
                  getAction("pad-dot")->setChecked(false);
                  getAction("pad-dot")->setEnabled(false);
                  // fall through
            case TDuration::DurationType::V_512TH:
                  getAction("pad-dotdot")->setChecked(false);
                  getAction("pad-dotdot")->setEnabled(false);
                  // fall through
            case TDuration::DurationType::V_256TH:
                  getAction("pad-dot3")->setChecked(false);
                  getAction("pad-dot3")->setEnabled(false);
                  // fall through
            case TDuration::DurationType::V_128TH:
                  getAction("pad-dot4")->setChecked(false);
                  getAction("pad-dot4")->setEnabled(false);
            default:
                  break;
            }

      getAction("note-longa")->setChecked(is.duration()  == TDuration::DurationType::V_LONG);
      getAction("note-breve")->setChecked(is.duration()  == TDuration::DurationType::V_BREVE);
      getAction("pad-note-1")->setChecked(is.duration()  == TDuration::DurationType::V_WHOLE);
      getAction("pad-note-2")->setChecked(is.duration()  == TDuration::DurationType::V_HALF);
      getAction("pad-note-4")->setChecked(is.duration()  == TDuration::DurationType::V_QUARTER);
      getAction("pad-note-8")->setChecked(is.duration()  == TDuration::DurationType::V_EIGHTH);
      getAction("pad-note-16")->setChecked(is.duration() == TDuration::DurationType::V_16TH);
      getAction("pad-note-32")->setChecked(is.duration() == TDuration::DurationType::V_32ND);
      getAction("pad-note-64")->setChecked(is.duration() == TDuration::DurationType::V_64TH);
      getAction("pad-note-128")->setChecked(is.duration() == TDuration::DurationType::V_128TH);

      // uncheck all voices if multi-selection
      int voice = score->selection().isSingle() ? is.voice() : -1;
      getAction("voice-1")->setChecked(voice == 0);
      getAction("voice-2")->setChecked(voice == 1);
      getAction("voice-3")->setChecked(voice == 2);
      getAction("voice-4")->setChecked(voice == 3);

      getAction("acciaccatura")->setChecked(is.noteType() == NoteType::ACCIACCATURA);
      getAction("appoggiatura")->setChecked(is.noteType() == NoteType::APPOGGIATURA);
      getAction("grace4")->setChecked(is.noteType()  == NoteType::GRACE4);
      getAction("grace16")->setChecked(is.noteType() == NoteType::GRACE16);
      getAction("grace32")->setChecked(is.noteType() == NoteType::GRACE32);
      getAction("grace8after")->setChecked(is.noteType()  == NoteType::GRACE8_AFTER);
      getAction("grace16after")->setChecked(is.noteType() == NoteType::GRACE16_AFTER);
      getAction("grace32after")->setChecked(is.noteType() == NoteType::GRACE32_AFTER);
      getAction("beam-start")->setChecked(is.beamMode() == Beam::Mode::BEGIN);
      getAction("beam-mid")->setChecked(is.beamMode()   == Beam::Mode::MID);
      getAction("no-beam")->setChecked(is.beamMode()    == Beam::Mode::NONE);
      getAction("beam32")->setChecked(is.beamMode()     == Beam::Mode::BEGIN32);
      getAction("auto-beam")->setChecked(is.beamMode()  == Beam::Mode::AUTO);

      if(is.noteEntryMode() && !is.rest())
            updateShadowNote();
      }
}

