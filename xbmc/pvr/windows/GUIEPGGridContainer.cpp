/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIEPGGridContainer.h"

#include <tinyxml.h>

#include "GUIInfoManager.h"
#include "guiinfo/GUIInfoLabels.h"
#include "guilib/DirtyRegion.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIListItem.h"
#include "input/Key.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "threads/SystemClock.h"

#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/Epg.h"
#include "pvr/windows/GUIEPGGridContainerModel.h"

using namespace PVR;

#define BLOCKJUMP    4 // how many blocks are jumped with each analogue scroll action
static const int BLOCK_SCROLL_OFFSET = 60 / CGUIEPGGridContainerModel::MINSPERBLOCK; // how many blocks are jumped if we are at left/right edge of grid

CGUIEPGGridContainer::CGUIEPGGridContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                                           ORIENTATION orientation, int scrollTime, int preloadItems, int timeBlocks, int rulerUnit,
                                           const CTextureInfo& progressIndicatorTexture)
: IGUIContainer(parentID, controlID, posX, posY, width, height),
  m_orientation(orientation),
  m_channelLayout(nullptr),
  m_focusedChannelLayout(nullptr),
  m_programmeLayout(nullptr),
  m_focusedProgrammeLayout(nullptr),
  m_rulerLayout(nullptr),
  m_rulerDateLayout(nullptr),
  m_pageControl(0),
  m_rulerUnit(rulerUnit),
  m_channelsPerPage(0),
  m_programmesPerPage(0),
  m_channelCursor(0),
  m_channelOffset(0),
  m_blocksPerPage(timeBlocks),
  m_blockCursor(0),
  m_blockOffset(0),
  m_blockTravelAxis(0),
  m_cacheChannelItems(preloadItems),
  m_cacheProgrammeItems(preloadItems),
  m_cacheRulerItems(preloadItems),
  m_rulerDateHeight(0),
  m_rulerDateWidth(0),
  m_rulerPosX(0),
  m_rulerPosY(0),
  m_rulerHeight(0),
  m_rulerWidth(0),
  m_channelPosX(0),
  m_channelPosY(0),
  m_channelHeight(0),
  m_channelWidth(0),
  m_gridPosX(0),
  m_gridPosY(0),
  m_gridWidth(0),
  m_gridHeight(0),
  m_blockSize(0),
  m_analogScrollCount(0),
  m_guiProgressIndicatorTexture(posX, posY, width, height, progressIndicatorTexture),
  m_scrollTime(scrollTime ? scrollTime : 1),
  m_programmeScrollLastTime(0),
  m_programmeScrollSpeed(0),
  m_programmeScrollOffset(0),
  m_channelScrollLastTime(0),
  m_channelScrollSpeed(0),
  m_channelScrollOffset(0),
  m_gridModel(new CGUIEPGGridContainerModel),
  m_item(nullptr)
{
  ControlType = GUICONTAINER_EPGGRID;
}

CGUIEPGGridContainer::CGUIEPGGridContainer(const CGUIEPGGridContainer &other)
: IGUIContainer(other),
  m_renderOffset(other.m_renderOffset),
  m_orientation(other.m_orientation),
  m_channelLayouts(other.m_channelLayouts),
  m_focusedChannelLayouts(other.m_focusedChannelLayouts),
  m_focusedProgrammeLayouts(other.m_focusedProgrammeLayouts),
  m_programmeLayouts(other.m_programmeLayouts),
  m_rulerLayouts(other.m_rulerLayouts),
  m_rulerDateLayouts(other.m_rulerDateLayouts),
  m_channelLayout(other.m_channelLayout),
  m_focusedChannelLayout(other.m_focusedChannelLayout),
  m_programmeLayout(other.m_programmeLayout),
  m_focusedProgrammeLayout(other.m_focusedProgrammeLayout),
  m_rulerLayout(other.m_rulerLayout),
  m_rulerDateLayout(other.m_rulerDateLayout),
  m_pageControl(other.m_pageControl),
  m_rulerUnit(other.m_rulerUnit),
  m_channelsPerPage(other.m_channelsPerPage),
  m_programmesPerPage(other.m_programmesPerPage),
  m_channelCursor(other.m_channelCursor),
  m_channelOffset(other.m_channelOffset),
  m_blocksPerPage(other.m_blocksPerPage),
  m_blockCursor(other.m_blockCursor),
  m_blockOffset(other.m_blockOffset),
  m_blockTravelAxis(other.m_blockTravelAxis),
  m_cacheChannelItems(other.m_cacheChannelItems),
  m_cacheProgrammeItems(other.m_cacheProgrammeItems),
  m_cacheRulerItems(other.m_cacheRulerItems),
  m_rulerDateHeight(other.m_rulerDateHeight),
  m_rulerDateWidth(other.m_rulerDateWidth),
  m_rulerPosX(other.m_rulerPosX),
  m_rulerPosY(other.m_rulerPosY),
  m_rulerHeight(other.m_rulerHeight),
  m_rulerWidth(other.m_rulerWidth),
  m_channelPosX(other.m_channelPosX),
  m_channelPosY(other.m_channelPosY),
  m_channelHeight(other.m_channelHeight),
  m_channelWidth(other.m_channelWidth),
  m_gridPosX(other.m_gridPosX),
  m_gridPosY(other.m_gridPosY),
  m_gridWidth(other.m_gridWidth),
  m_gridHeight(other.m_gridHeight),
  m_blockSize(other.m_blockSize),
  m_analogScrollCount(other.m_analogScrollCount),
  m_guiProgressIndicatorTexture(other.m_guiProgressIndicatorTexture),
  m_lastItem(other.m_lastItem),
  m_lastChannel(other.m_lastChannel),
  m_scrollTime(other.m_scrollTime),
  m_programmeScrollLastTime(other.m_programmeScrollLastTime),
  m_programmeScrollSpeed(other.m_programmeScrollSpeed),
  m_programmeScrollOffset(other.m_programmeScrollOffset),
  m_channelScrollLastTime(other.m_channelScrollLastTime),
  m_channelScrollSpeed(other.m_channelScrollSpeed),
  m_channelScrollOffset(other.m_channelScrollOffset),
  m_gridModel(new CGUIEPGGridContainerModel(*other.m_gridModel)),
  m_updatedGridModel(other.m_updatedGridModel ? new CGUIEPGGridContainerModel(*other.m_updatedGridModel) : nullptr),
  m_outdatedGridModel(other.m_outdatedGridModel ? new CGUIEPGGridContainerModel(*other.m_outdatedGridModel) : nullptr),
  m_item(GetItem(m_channelCursor)) // pointer to grid model internal data.
{
}

void CGUIEPGGridContainer::AllocResources()
{
  IGUIContainer::AllocResources();
  m_guiProgressIndicatorTexture.AllocResources();
}

void CGUIEPGGridContainer::FreeResources(bool immediately)
{
  m_guiProgressIndicatorTexture.FreeResources(immediately);
  IGUIContainer::FreeResources(immediately);
}

void CGUIEPGGridContainer::SetPageControl(int id)
{
  m_pageControl = id;
}

void CGUIEPGGridContainer::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  ValidateOffset();

  if (m_bInvalidated)
  {
    UpdateLayout();

    if (m_pageControl)
    {
      int iItemsPerPage;
      int iTotalItems;

      if (m_orientation == VERTICAL)
      {
        iItemsPerPage = m_channelsPerPage;
        iTotalItems = m_gridModel->ChannelItemsSize();
      }
      else
      {
        iItemsPerPage = m_blocksPerPage;
        iTotalItems = m_gridModel->GetBlockCount();
      }

      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, iItemsPerPage, iTotalItems);
      SendWindowMessage(msg);
    }
  }

  UpdateScrollOffset(currentTime);
  ProcessChannels(currentTime, dirtyregions);
  ProcessRulerDate(currentTime, dirtyregions);
  ProcessRuler(currentTime, dirtyregions);
  ProcessProgrammeGrid(currentTime, dirtyregions);
  ProcessProgressIndicator(currentTime, dirtyregions);

  if (m_pageControl)
  {
    int iItem = (m_orientation == VERTICAL)
      ? MathUtils::round_int(m_channelScrollOffset / m_channelHeight)
      : MathUtils::round_int(m_programmeScrollOffset / (m_gridHeight / m_blocksPerPage));

    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, iItem);
    SendWindowMessage(msg);
  }

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIEPGGridContainer::Render()
{
  RenderChannels();
  RenderRulerDate();
  RenderRuler();
  RenderProgrammeGrid();
  RenderProgressIndicator();

  CGUIControl::Render();
}

void CGUIEPGGridContainer::ProcessChannels(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  HandleChannels(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderChannels()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleChannels(true, dummyTime, dummyRegions);
}

void CGUIEPGGridContainer::ProcessRulerDate(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  HandleRulerDate(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderRulerDate()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleRulerDate(true, dummyTime, dummyRegions);
}

void CGUIEPGGridContainer::ProcessRuler(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  HandleRuler(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderRuler()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleRuler(true, dummyTime, dummyRegions);
}

void CGUIEPGGridContainer::ProcessProgrammeGrid(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  HandleProgrammeGrid(false, currentTime, dirtyregions);
}

void CGUIEPGGridContainer::RenderProgrammeGrid()
{
  // params not needed for render.
  unsigned int dummyTime = 0;
  CDirtyRegionList dummyRegions;
  HandleProgrammeGrid(true, dummyTime, dummyRegions);
}

float CGUIEPGGridContainer::GetCurrentTimePositionOnPage() const
{
  if (!m_gridModel->GetGridStart().IsValid())
    return -1.0f;

  const CDateTimeSpan startDelta(CDateTime::GetUTCDateTime() - m_gridModel->GetGridStart());
  float fPos = (startDelta.GetSecondsTotal() * m_blockSize) / (CGUIEPGGridContainerModel::MINSPERBLOCK * 60) - m_programmeScrollOffset;
  return std::min(fPos, m_orientation == VERTICAL ? m_gridWidth : m_gridHeight);
}

float CGUIEPGGridContainer::GetProgressIndicatorWidth() const
{
  return (m_orientation == VERTICAL) ? GetCurrentTimePositionOnPage() : m_rulerWidth + m_gridWidth;
}

float CGUIEPGGridContainer::GetProgressIndicatorHeight() const
{
  return (m_orientation == VERTICAL) ? m_rulerHeight + m_gridHeight : GetCurrentTimePositionOnPage();
}

void CGUIEPGGridContainer::ProcessProgressIndicator(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  float width = GetProgressIndicatorWidth();
  float height = GetProgressIndicatorHeight();

  if (width > 0 && height > 0)
  {
    m_guiProgressIndicatorTexture.SetVisible(true);
    m_guiProgressIndicatorTexture.SetPosition(m_rulerPosX + m_renderOffset.x, m_rulerPosY + m_renderOffset.y);
    m_guiProgressIndicatorTexture.SetWidth(width);
    m_guiProgressIndicatorTexture.SetHeight(height);
  }
  else
  {
    m_guiProgressIndicatorTexture.SetVisible(false);
  }
  
  m_guiProgressIndicatorTexture.Process(currentTime);
}

void CGUIEPGGridContainer::RenderProgressIndicator()
{
  if (g_graphicsContext.SetClipRegion(m_rulerPosX, m_rulerPosY, GetProgressIndicatorWidth(), GetProgressIndicatorHeight()))
  {
    m_guiProgressIndicatorTexture.SetDiffuseColor(m_diffuseColor);
    m_guiProgressIndicatorTexture.Render();
    g_graphicsContext.RestoreClipRegion();
  }
}

void CGUIEPGGridContainer::ProcessItem(float posX, float posY, const CFileItemPtr &item, CFileItemPtr &lastitem,
  bool focused, CGUIListItemLayout* normallayout, CGUIListItemLayout* focusedlayout,
  unsigned int currentTime, CDirtyRegionList &dirtyregions, float resize /* = -1.0f */)
{
  if (!normallayout || !focusedlayout)
    return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();

  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*focusedlayout);
      item->SetFocusedLayout(layout);
    }

    if (resize != -1.0f)
    {
      if (m_orientation == VERTICAL)
        item->GetFocusedLayout()->SetWidth(resize);
      else
        item->GetFocusedLayout()->SetHeight(resize);
    }

    if (item != lastitem || !HasFocus())
      item->GetFocusedLayout()->SetFocusedItem(0);

    if (item != lastitem && HasFocus())
    {
      item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);

      unsigned int subItem = 1;
      if (lastitem && lastitem->GetFocusedLayout())
        subItem = lastitem->GetFocusedLayout()->GetFocusedItem();

      item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
    }

    item->GetFocusedLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
    lastitem = item;
  }
  else
  {
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*normallayout);
      item->SetLayout(layout);
    }

    if (resize != -1.0f)
    {
      if (m_orientation == VERTICAL)
        item->GetLayout()->SetWidth(resize);
      else
        item->GetLayout()->SetHeight(resize);
    }

    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);

    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
    else
      item->GetLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
  }
  g_graphicsContext.RestoreOrigin();
}

void CGUIEPGGridContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (focused)
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->Render(item, m_parentID);
  }
  else
  {
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Render(item, m_parentID);
    else if (item->GetLayout())
      item->GetLayout()->Render(item, m_parentID);
  }
  g_graphicsContext.RestoreOrigin();
}

bool CGUIEPGGridContainer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_DOWN:
    case ACTION_MOVE_UP:
    case ACTION_NAV_BACK:
      // use base class implementation
      return CGUIControl::OnAction(action);

    case ACTION_NEXT_ITEM:
      // skip +12h
      ScrollToBlockOffset(m_blockOffset + (12 * 60  / CGUIEPGGridContainerModel::MINSPERBLOCK));
      return true;

    case ACTION_PREV_ITEM:
      // skip -12h
      ScrollToBlockOffset(m_blockOffset - (12 * 60 / CGUIEPGGridContainerModel::MINSPERBLOCK));
      return true;

    case REMOTE_0:
      GoToNow();
      return true;

    case ACTION_PAGE_UP:
      if (m_orientation == VERTICAL)
      {
        if (m_channelOffset == 0)
        {
          // already on the first page, so move to the first item
          SetChannel(0);
        }
        else
        {
          // scroll up to the previous page
          ChannelScroll(-m_channelsPerPage);
        }
      }
      else
      {
        if (m_blockOffset == 0)
        {
          // already on the first page, so move to the first item
          SetBlock(0);
        }
        else
        {
          // scroll up to the previous page
          ProgrammesScroll(-m_blocksPerPage);
        }
      }
      return true;

    case ACTION_PAGE_DOWN:
      if (m_orientation == VERTICAL)
      {
        if (m_channelOffset == m_gridModel->ChannelItemsSize() - m_channelsPerPage || m_gridModel->ChannelItemsSize() < m_channelsPerPage)
        {
          // already at the last page, so move to the last item.
          SetChannel(m_gridModel->ChannelItemsSize() - m_channelOffset - 1);
        }
        else
        {
          // scroll down to the next page
          ChannelScroll(m_channelsPerPage);
        }
      }
      else
      {
        if (m_blockOffset == m_gridModel->GetBlockCount() - m_blocksPerPage || m_gridModel->GetBlockCount() < m_blocksPerPage)
        {
          // already at the last page, so move to the last item.
          SetBlock(m_gridModel->GetBlockCount() - m_blockOffset - 1);
        }
        else
        {
          // scroll down to the next page
          ProgrammesScroll(m_blocksPerPage);
        }
      }

      return true;

    // smooth scrolling (for analog controls)
    case ACTION_TELETEXT_RED:
    case ACTION_TELETEXT_GREEN:
    case ACTION_SCROLL_UP: // left horizontal scrolling
      if (m_orientation == VERTICAL)
      {
        int blocksToJump = action.GetID() == ACTION_TELETEXT_RED ? m_blocksPerPage / 2 : m_blocksPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_blockOffset > 0 && m_blockCursor <= m_blocksPerPage / 2)
            ProgrammesScroll(-blocksToJump);
          else if (m_blockCursor > blocksToJump)
            SetBlock(m_blockCursor - blocksToJump);
        }
        return handled;
      }
      else
      {
        int channelsToJump = action.GetID() == ACTION_TELETEXT_RED ? m_channelsPerPage / 2 : m_channelsPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_channelOffset > 0 && m_channelCursor <= m_channelsPerPage / 2)
            ChannelScroll(-channelsToJump);
          else if (m_channelCursor > channelsToJump)
            SetChannel(m_channelCursor - channelsToJump);
        }
        return handled;
      }
      break;

    case ACTION_TELETEXT_BLUE:
    case ACTION_TELETEXT_YELLOW:
    case ACTION_SCROLL_DOWN: // right horizontal scrolling
      if (m_orientation == VERTICAL)
      {
        int blocksToJump = action.GetID() == ACTION_TELETEXT_BLUE ? m_blocksPerPage / 2 : m_blocksPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_blockOffset + m_blocksPerPage < m_gridModel->GetBlockCount() && m_blockCursor >= m_blocksPerPage / 2)
            ProgrammesScroll(blocksToJump);
          else if (m_blockCursor < m_blocksPerPage - blocksToJump && m_blockOffset + m_blockCursor < m_gridModel->GetBlockCount() - blocksToJump)
            SetBlock(m_blockCursor + blocksToJump);
        }
        return handled;
      }
      else
      {
        int channelsToJump = action.GetID() == ACTION_TELETEXT_BLUE ? m_channelsPerPage / 2 : m_channelsPerPage / 4;

        m_analogScrollCount += action.GetAmount() * action.GetAmount();
        bool handled = false;

        while (m_analogScrollCount > 0.4)
        {
          handled = true;
          m_analogScrollCount -= 0.4f;

          if (m_channelOffset + m_channelsPerPage < m_gridModel->ChannelItemsSize() && m_channelCursor >= m_channelsPerPage / 2)
            ChannelScroll(channelsToJump);
          else if (m_channelCursor < m_channelsPerPage - channelsToJump && m_channelOffset + m_channelCursor < m_gridModel->ChannelItemsSize() - channelsToJump)
            SetChannel(m_channelCursor + channelsToJump);
        }
        return handled;
      }
      break;

    default:
      if (action.GetID())
        return OnClick(action.GetID());

      break;
  }

  return false;
}

bool CGUIEPGGridContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    switch (message.GetMessage())
    {
      case GUI_MSG_ITEM_SELECTED:
        message.SetParam1(GetSelectedItem());
        return true;

      case GUI_MSG_PAGE_CHANGE:
        if (message.GetSenderId() == m_pageControl && IsVisible())
        {
          if (m_orientation == VERTICAL)
            ScrollToChannelOffset(message.GetParam1());
          else
            ScrollToBlockOffset(message.GetParam1());

          return true;
        }
        break;

      case GUI_MSG_LABEL_BIND:
        UpdateItems();
        return true;

      case GUI_MSG_REFRESH_LIST:
        // update our list contents
        m_gridModel->SetInvalid();
        break;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIEPGGridContainer::UpdateItems()
{
  CSingleLock lock(m_critSection);

  if (!m_updatedGridModel)
    return;

  /* Safe currently selected epg tag and grid coordinates. Selection shall be restored after update. */
  CPVREpgInfoTagPtr prevSelectedEpgTag(GetSelectedEpgInfoTag());
  const int oldChannelIndex = m_channelOffset + m_channelCursor;
  const int oldBlockIndex   = m_blockOffset + m_blockCursor;
  const CDateTime oldGridStart(m_gridModel->GetGridStart());
  int eventOffset           = oldBlockIndex;
  int newChannelIndex       = oldChannelIndex;
  int newBlockIndex         = oldBlockIndex;
  int channelUid            = -1;
  unsigned int broadcastUid = 0;

  if (prevSelectedEpgTag)
  {
    // get the block offset relative to the first block of the selected event
    while (eventOffset > 0)
    {
      if (m_gridModel->GetGridItem(oldChannelIndex, eventOffset - 1) != m_gridModel->GetGridItem(oldChannelIndex, oldBlockIndex))
        break;

      eventOffset--;
    }

    eventOffset = oldBlockIndex - eventOffset;

    if (prevSelectedEpgTag->StartAsUTC().IsValid() && prevSelectedEpgTag->EndAsUTC().IsValid()) // "normal" tag selected
    {
      const CDateTime gridStart(m_gridModel->GetGridStart());
      const CDateTime eventStart(prevSelectedEpgTag->StartAsUTC());

      if (gridStart >= eventStart)
      {
        // start of previously selected event is before grid start
        newBlockIndex = eventOffset;
      }
      else
        newBlockIndex = (eventStart - gridStart).GetSecondsTotal() / 60 / CGUIEPGGridContainerModel::MINSPERBLOCK + eventOffset;

      const CPVRChannelPtr channel(prevSelectedEpgTag->ChannelTag());
      if (channel)
        channelUid = channel->UniqueID();

      broadcastUid = prevSelectedEpgTag->UniqueBroadcastID();
    }
    else // "gap" tag selected
    {
      const GridItem *currItem(GetItem(m_channelCursor));
      if (currItem)
        channelUid = currItem->item->GetEPGInfoTag()->ChannelTag()->UniqueID();

      const GridItem *prevItem(GetPrevItem(m_channelCursor));
      if (prevItem)
      {
        const CPVREpgInfoTagPtr tag(prevItem->item->GetEPGInfoTag());
        if (tag && tag->EndAsUTC().IsValid())
        {
          const CDateTime gridStart(m_gridModel->GetGridStart());
          const CDateTime eventEnd(tag->EndAsUTC());

          if (gridStart >= eventEnd)
          {
            // start of previously selected gap tag is before grid start
            newBlockIndex = eventOffset;
          }
          else
            newBlockIndex = (eventEnd - gridStart).GetSecondsTotal() / 60 / CGUIEPGGridContainerModel::MINSPERBLOCK + eventOffset;

          broadcastUid = tag->UniqueBroadcastID();
        }
      }
    }
  }

  m_lastItem    = nullptr;
  m_lastChannel = nullptr;

  // always use asynchronously precalculated grid data.
  m_outdatedGridModel = std::move(m_gridModel); // destructing grid data can be very expensive, thus this will be done asynchronously, not here.
  m_gridModel = std::move(m_updatedGridModel);

  if (prevSelectedEpgTag)
  {
    if (oldGridStart != m_gridModel->GetGridStart())
    {
      // grid start changed. block offset for selected event might have changed.
      int diff;
      if (m_gridModel->GetGridStart() > oldGridStart)
        diff = -(m_gridModel->GetGridStart() - oldGridStart).GetSecondsTotal();
      else
        diff = (oldGridStart - m_gridModel->GetGridStart()).GetSecondsTotal();

      newBlockIndex += diff / 60 / CGUIEPGGridContainerModel::MINSPERBLOCK;
      if (newBlockIndex < 0 || newBlockIndex + 1 > m_gridModel->GetBlockCount())
      {
        // previous selection is no longer in grid.
        SetInvalid();
        GoToChannel(newChannelIndex);
        GoToNow();
        return;
      }
    }
  }

  if (prevSelectedEpgTag && (oldChannelIndex != 0 || oldBlockIndex != 0))
  {
    if (newChannelIndex >= m_gridModel->ChannelItemsSize() ||
        newBlockIndex >= m_gridModel->GetBlockCount() ||
        m_gridModel->GetGridItem(newChannelIndex, newBlockIndex)->GetEPGInfoTag() != prevSelectedEpgTag)
    {
      int iChannelIndex = CGUIEPGGridContainerModel::INVALID_INDEX;
      int iBlockIndex = CGUIEPGGridContainerModel::INVALID_INDEX;
      m_gridModel->FindChannelAndBlockIndex(channelUid, broadcastUid, eventOffset, iChannelIndex, iBlockIndex);

      if (iChannelIndex != CGUIEPGGridContainerModel::INVALID_INDEX)
      {
        newChannelIndex = iChannelIndex;
      }
      else if (newChannelIndex >= m_gridModel->ChannelItemsSize() ||
               m_gridModel->GetGridItem(newChannelIndex, newBlockIndex)->GetEPGInfoTag()->ChannelTag() != prevSelectedEpgTag->ChannelTag())
      {
        // default to first channel
        newChannelIndex = 0;
      }

      if (iBlockIndex != CGUIEPGGridContainerModel::INVALID_INDEX)
      {
        newBlockIndex = iBlockIndex;
      }
      else if (newBlockIndex >= m_gridModel->GetBlockCount())
      {
        // default to now
        const CDateTime currentDate = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
        newBlockIndex = (currentDate - m_gridModel->GetGridStart()).GetSecondsTotal() / 60 / CGUIEPGGridContainerModel::MINSPERBLOCK - GetPageNowOffset();
      }
    }

    // restore previous selection.
    if (newChannelIndex == oldChannelIndex && newBlockIndex == oldBlockIndex)
    {
      // same coordinates, keep current grid view port
      m_item = GetItem(m_channelCursor);
    }
    else
    {
      // new coordinates, move grid view port accordingly
      SetInvalid();

      if (newBlockIndex != oldBlockIndex)
        GoToBlock(newBlockIndex);

      if (newChannelIndex != oldChannelIndex)
        GoToChannel(newChannelIndex);
    }
  }
  else
  {
    // no previous selection, goto now
    m_item = GetItem(m_channelCursor);

    SetInvalid();
    GoToNow();
  }
}

void CGUIEPGGridContainer::ChannelScroll(int amount)
{
  // increase or decrease the vertical offset
  int offset = m_channelOffset + amount;

  if (offset > m_gridModel->ChannelItemsSize() - m_channelsPerPage)
    offset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;

  if (offset < 0)
    offset = 0;

  ScrollToChannelOffset(offset);
}

void CGUIEPGGridContainer::ProgrammesScroll(int amount)
{
  // increase or decrease the horizontal offset
  ScrollToBlockOffset(m_blockOffset + amount);
}

void CGUIEPGGridContainer::OnUp()
{
  if (m_orientation == VERTICAL)
  {
    CGUIAction action = GetAction(ACTION_MOVE_UP);
    if (m_channelCursor > 0)
    {
      SetChannel(m_channelCursor - 1);
    }
    else if (m_channelCursor == 0 && m_channelOffset)
    {
      ScrollToChannelOffset(m_channelOffset - 1);
      SetChannel(0);
    }
    else if (action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition()) // wrap around
    {
      int offset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;

      if (offset < 0)
        offset = 0;

      SetChannel(m_gridModel->ChannelItemsSize() - offset - 1);
      ScrollToChannelOffset(offset);
    }
    else
      CGUIControl::OnUp();
  }
  else
  {
    if (m_gridModel->HasGridItems() && m_item)
    {
      if (m_channelCursor + m_channelOffset >= 0 && m_blockOffset >= 0 &&
          m_item->item != m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blockOffset))
      {
        // this is not first item on page
        m_item = GetPrevItem(m_channelCursor);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
      else if (m_blockCursor <= 0 && m_blockOffset && m_blockOffset - BLOCK_SCROLL_OFFSET >= 0)
      {
        // this is the first item on page
        ScrollToBlockOffset(m_blockOffset - BLOCK_SCROLL_OFFSET);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
    }

    CGUIControl::OnUp();
  }
}

void CGUIEPGGridContainer::OnDown()
{
  if (m_orientation == VERTICAL)
  {
    CGUIAction action = GetAction(ACTION_MOVE_DOWN);
    if (m_channelOffset + m_channelCursor + 1 < m_gridModel->ChannelItemsSize())
    {
      if (m_channelCursor + 1 < m_channelsPerPage)
      {
        SetChannel(m_channelCursor + 1);
      }
      else
      {
        ScrollToChannelOffset(m_channelOffset + 1);
        SetChannel(m_channelsPerPage - 1);
      }
    }
    else if (action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition()) // wrap around
    {
      ScrollToChannelOffset(0);
      SetChannel(0);
    }
    else
      CGUIControl::OnDown();
  }
  else
  {
    if (m_gridModel->HasGridItems() && m_item)
    {
      if (m_item->item != m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blocksPerPage + m_blockOffset - 1))
      {
        // this is not last item on page
        m_item = GetNextItem(m_channelCursor);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
      else if ((m_blockOffset != m_gridModel->GetBlockCount() - m_blocksPerPage) &&
               m_gridModel->GetBlockCount() > m_blocksPerPage &&
               m_blockOffset + BLOCK_SCROLL_OFFSET <= m_gridModel->GetBlockCount())
      {
        // this is the last item on page
        ScrollToBlockOffset(m_blockOffset + BLOCK_SCROLL_OFFSET);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
    }

    CGUIControl::OnDown();
  }
}

void CGUIEPGGridContainer::OnLeft()
{
  if (m_orientation == VERTICAL)
  {
    if (m_gridModel->HasGridItems() && m_item)
    {
      if (m_channelCursor + m_channelOffset >= 0 && m_blockOffset >= 0 &&
          m_item->item != m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blockOffset))
      {
        // this is not first item on page
        m_item = GetPrevItem(m_channelCursor);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
      else if (m_blockCursor <= 0 && m_blockOffset && m_blockOffset - BLOCK_SCROLL_OFFSET >= 0)
      {
        // this is the first item on page
        ScrollToBlockOffset(m_blockOffset - BLOCK_SCROLL_OFFSET);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
    }

    CGUIControl::OnLeft();
  }
  else
  {
    CGUIAction action = GetAction(ACTION_MOVE_LEFT);
    if (m_channelCursor > 0)
    {
      SetChannel(m_channelCursor - 1);
    }
    else if (m_channelCursor == 0 && m_channelOffset)
    {
      ScrollToChannelOffset(m_channelOffset - 1);
      SetChannel(0);
    }
    else if (action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition()) // wrap around
    {
      int offset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;

      if (offset < 0)
        offset = 0;

      SetChannel(m_gridModel->ChannelItemsSize() - offset - 1);
      ScrollToChannelOffset(offset);
    }
    else
      CGUIControl::OnLeft();
  }
}

void CGUIEPGGridContainer::OnRight()
{
  if (m_orientation == VERTICAL)
  {
    if (m_gridModel->HasGridItems() && m_item)
    {
      if (m_item->item != m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blocksPerPage + m_blockOffset - 1))
      {
        // this is not last item on page
        m_item = GetNextItem(m_channelCursor);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
      else if ((m_blockOffset != m_gridModel->GetBlockCount() - m_blocksPerPage) &&
               m_gridModel->GetBlockCount() > m_blocksPerPage &&
               m_blockOffset + BLOCK_SCROLL_OFFSET <= m_gridModel->GetBlockCount())
      {
        // this is the last item on page
        ScrollToBlockOffset(m_blockOffset + BLOCK_SCROLL_OFFSET);
        SetBlock(GetBlock(m_item->item, m_channelCursor));

        return;
      }
    }

    CGUIControl::OnRight();
  }
  else
  {
    CGUIAction action = GetAction(ACTION_MOVE_RIGHT);
    if (m_channelOffset + m_channelCursor + 1 < m_gridModel->ChannelItemsSize())
    {
      if (m_channelCursor + 1 < m_channelsPerPage)
      {
        SetChannel(m_channelCursor + 1);
      }
      else
      {
        ScrollToChannelOffset(m_channelOffset + 1);
        SetChannel(m_channelsPerPage - 1);
      }
    }
    else if (action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition()) // wrap around
    {
      SetChannel(0);
      ScrollToChannelOffset(0);
    }
    else
      CGUIControl::OnRight();
  }
}

bool CGUIEPGGridContainer::SetChannel(const std::string &channel)
{
  for (int iIndex = 0; iIndex < m_gridModel->ChannelItemsSize(); iIndex++)
  {
    std::string strPath = m_gridModel->GetChannelItem(iIndex)->GetProperty("path").asString();
    if (strPath == channel)
    {
      GoToChannel(iIndex);
      return true;
    }
  }
  return false;
}

bool CGUIEPGGridContainer::SetChannel(const CPVRChannelPtr &channel)
{
  for (int iIndex = 0; iIndex < m_gridModel->ChannelItemsSize(); iIndex++)
  {
    int iChannelId = static_cast<int>(m_gridModel->GetChannelItem(iIndex)->GetProperty("channelid").asInteger(-1));
    if (iChannelId == channel->ChannelID())
    {
      GoToChannel(iIndex);
      return true;
    }
  }
  return false;
}

void CGUIEPGGridContainer::SetChannel(int channel)
{
  CSingleLock lock(m_critSection);

  int channelIndex = channel + m_channelOffset;
  int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex < m_gridModel->ChannelItemsSize() && blockIndex < m_gridModel->GetBlockCount())
  {
    m_item = m_gridModel->GetGridItemPtr(channelIndex, m_blockTravelAxis);
    if (m_item)
    {
      m_channelCursor = channel;
      MarkDirtyRegion();
      SetBlock(GetBlock(m_item->item, channel), false);
    }
  }
}

void CGUIEPGGridContainer::SetBlock(int block, bool bUpdateBlockTravelAxis /* = true */)
{
  CSingleLock lock(m_critSection);

  if (block < 0)
    m_blockCursor = 0;
  else if (block > m_blocksPerPage - 1)
    m_blockCursor = m_blocksPerPage - 1;
  else
    m_blockCursor = block;

  if (bUpdateBlockTravelAxis)
    m_blockTravelAxis = m_blockOffset + m_blockCursor;

  m_item = GetItem(m_channelCursor);
  MarkDirtyRegion();
}

CGUIListItemLayout *CGUIEPGGridContainer::GetFocusedLayout() const
{
  CGUIListItemPtr item = GetListItem(0);

  if (item)
    return item->GetFocusedLayout();

  return nullptr;
}

bool CGUIEPGGridContainer::SelectItemFromPoint(const CPoint &point, bool justGrid /* = false */)
{
  /* point has already had origin set to m_posX, m_posY */
  if (!m_focusedProgrammeLayout || !m_programmeLayout || (justGrid && point.x < 0))
    return false;

  int channel;
  int block;

  if (m_orientation == VERTICAL)
  {
    channel = point.y / m_channelHeight;
    block = point.x / m_blockSize;
  }
  else
  {
    channel = point.x / m_channelWidth;
    block = point.y / m_blockSize;
  }

  if (channel > m_channelsPerPage)
    channel = m_channelsPerPage - 1;

  if (channel >= m_gridModel->ChannelItemsSize())
    channel = m_gridModel->ChannelItemsSize() - 1;

  if (channel < 0)
    channel = 0;

  if (block > m_blocksPerPage)
    block = m_blocksPerPage - 1;

  if (block < 0)
    block = 0;

  int channelIndex = channel + m_channelOffset;
  int blockIndex = block + m_blockOffset;

  // bail if out of range
  if (channelIndex >= m_gridModel->ChannelItemsSize() || blockIndex >= m_gridModel->GetBlockCount())
    return false;

  // bail if block isn't occupied
  if (!m_gridModel->GetGridItem(channelIndex, blockIndex))
    return false;

  SetChannel(channel);
  SetBlock(block);
  return true;
}

EVENT_RESULT CGUIEPGGridContainer::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  switch (event.m_id)
  {
  case ACTION_MOUSE_LEFT_CLICK:
    OnMouseClick(0, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_RIGHT_CLICK:
    OnMouseClick(1, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_DOUBLE_CLICK:
    OnMouseDoubleClick(0, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_WHEEL_UP:
    OnMouseWheel(-1, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_MOUSE_WHEEL_DOWN:
    OnMouseWheel(1, point);
    return EVENT_RESULT_HANDLED;
  case ACTION_GESTURE_BEGIN:
    {
      // we want exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
      return EVENT_RESULT_HANDLED;
    }
  case ACTION_GESTURE_END:
    {
      // we're done with exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
      ScrollToChannelOffset(MathUtils::round_int(m_channelScrollOffset / m_channelLayout->Size(m_orientation)));
      ScrollToBlockOffset(MathUtils::round_int(m_programmeScrollOffset / m_blockSize));
      return EVENT_RESULT_HANDLED;
    }
  case ACTION_GESTURE_PAN:
    {
      m_programmeScrollOffset -= event.m_offsetX;
      m_channelScrollOffset -= event.m_offsetY;

      {
        CSingleLock lock(m_critSection);

        m_channelOffset = MathUtils::round_int(m_channelScrollOffset / m_channelLayout->Size(m_orientation));
        m_blockOffset = MathUtils::round_int(m_programmeScrollOffset / m_blockSize);
        ValidateOffset();
      }
      return EVENT_RESULT_HANDLED;
    }
  default:
    return EVENT_RESULT_UNHANDLED;
  }
}

bool CGUIEPGGridContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  SelectItemFromPoint(point - CPoint(m_gridPosX, m_gridPosY), false);
  return CGUIControl::OnMouseOver(point);
}

bool CGUIEPGGridContainer::OnMouseClick(int dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_gridPosX, m_gridPosY)))
  {
    // send click message to window
    OnClick(ACTION_MOUSE_LEFT_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIEPGGridContainer::OnMouseDoubleClick(int dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_gridPosX, m_gridPosY)))
  {
    // send double click message to window
    OnClick(ACTION_MOUSE_DOUBLE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIEPGGridContainer::OnClick(int actionID)
{
  int subItem = 0;

  if (actionID == ACTION_SELECT_ITEM || actionID == ACTION_MOUSE_LEFT_CLICK)
  {
    // grab the currently focused subitem (if applicable)
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();

    if (focusedLayout)
      subItem = focusedLayout->GetFocusedItem();
  }

  // Don't know what to do, so send to our parent window.
  CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), actionID, subItem);
  return SendWindowMessage(msg);
}

bool CGUIEPGGridContainer::OnMouseWheel(char wheel, const CPoint &point)
{
  // doesn't work while an item is selected?
  ProgrammesScroll(-wheel);
  return true;
}

CPVRChannelPtr CGUIEPGGridContainer::GetSelectedChannel()
{
  CFileItemPtr fileItem;
  {
    CSingleLock lock(m_critSection);
    if (m_channelCursor + m_channelOffset < m_gridModel->ChannelItemsSize())
      fileItem = m_gridModel->GetChannelItem(m_channelCursor + m_channelOffset);
  }

  if (fileItem && fileItem->HasPVRChannelInfoTag())
    return fileItem->GetPVRChannelInfoTag();

  return CPVRChannelPtr();
}

int CGUIEPGGridContainer::GetSelectedItem() const
{
  if (!m_gridModel->HasGridItems() ||
      !m_gridModel->HasChannelItems() ||
      m_channelCursor + m_channelOffset >= m_gridModel->ChannelItemsSize() ||
      m_blockCursor + m_blockOffset >= m_gridModel->GetBlockCount())
    return -1;

  return m_gridModel->GetGridItemIndex(m_channelCursor + m_channelOffset, m_blockCursor + m_blockOffset);
}

CFileItemPtr CGUIEPGGridContainer::GetSelectedChannelItem() const
{
  CFileItemPtr item;

  if (m_gridModel->HasGridItems() &&
      m_gridModel->ChannelItemsSize() > 0 &&
      m_channelCursor + m_channelOffset < m_gridModel->ChannelItemsSize() &&
      m_blockCursor + m_blockOffset < m_gridModel->GetBlockCount())
    item = m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blockCursor + m_blockOffset);

  return item;
}

CPVREpgInfoTagPtr CGUIEPGGridContainer::GetSelectedEpgInfoTag() const
{
  CPVREpgInfoTagPtr tag;

  if (m_gridModel->HasGridItems() &&
      m_gridModel->HasChannelItems() &&
      m_channelCursor + m_channelOffset < m_gridModel->ChannelItemsSize() &&
      m_blockCursor + m_blockOffset < m_gridModel->GetBlockCount())
  {
    CFileItemPtr currentItem(m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, m_blockCursor + m_blockOffset));
    if (currentItem)
      tag = currentItem->GetEPGInfoTag();
  }

  return tag;
}

unsigned int CGUIEPGGridContainer::GetPageNowOffset() const
{
  return m_gridModel->GetGridStartPadding() / CGUIEPGGridContainerModel::MINSPERBLOCK; // this is the 'now' block relative to page start
}

CGUIListItemPtr CGUIEPGGridContainer::GetListItem(int offset, unsigned int flag) const
{
  if (!m_gridModel->HasChannelItems())
    return CGUIListItemPtr();

  int item = m_channelCursor + m_channelOffset + offset;
  if (flag & INFOFLAG_LISTITEM_POSITION)
    item = MathUtils::round_int(m_channelScrollOffset / m_channelLayout->Size(m_orientation));

  if (flag & INFOFLAG_LISTITEM_WRAP)
  {
    item %= m_gridModel->ChannelItemsSize();
    if (item < 0)
      item += m_gridModel->ChannelItemsSize();

    return m_gridModel->GetChannelItem(item);
  }
  else
  {
    if (item >= 0 && item < m_gridModel->ChannelItemsSize())
      return m_gridModel->GetChannelItem(item);
  }
  return CGUIListItemPtr();
}

std::string CGUIEPGGridContainer::GetLabel(int info) const
{
  std::string label;
  switch (info)
  {
  case CONTAINER_NUM_PAGES:
    if (m_channelsPerPage > 0)
      label = StringUtils::Format("%u", (m_gridModel->ChannelItemsSize() + m_channelsPerPage - 1) / m_channelsPerPage);
    else
      label = StringUtils::Format("%u", 0);
    break;
  case CONTAINER_CURRENT_PAGE:
    if (m_channelsPerPage > 0)
      label = StringUtils::Format("%u", 1 + (m_channelCursor + m_channelOffset) / m_channelsPerPage);
    else
      label = StringUtils::Format("%u", 1);
    break;
  case CONTAINER_POSITION:
    label = StringUtils::Format("%i", 1 + m_channelCursor + m_channelOffset);
    break;
  case CONTAINER_NUM_ITEMS:
    label = StringUtils::Format("%u", m_gridModel->ChannelItemsSize());
    break;
  default:
      break;
  }
  return label;
}

int CGUIEPGGridContainer::GetBlock(const CGUIListItemPtr &item, int channel)
{
  if (!item)
    return 0;

  return GetRealBlock(item, channel) - m_blockOffset;
}

int CGUIEPGGridContainer::GetRealBlock(const CGUIListItemPtr &item, int channel)
{
  int channelIndex = channel + m_channelOffset;
  int block = 0;

  while (block < m_gridModel->GetBlockCount() && m_gridModel->GetGridItem(channelIndex, block) != item)
    block++;

  return block;
}

GridItem *CGUIEPGGridContainer::GetNextItem(int channel)
{
  const int channelIndex = channel + m_channelOffset;
  const int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex >= m_gridModel->ChannelItemsSize() || blockIndex >= m_gridModel->GetBlockCount())
    return nullptr;

  int i = m_blockCursor;

  while (i < m_blocksPerPage &&
         i + m_blockOffset + 1 < m_gridModel->GetBlockCount() &&
         m_gridModel->GetGridItem(channelIndex, i + m_blockOffset) == m_gridModel->GetGridItem(channelIndex, blockIndex))
    i++;

  return m_gridModel->GetGridItemPtr(channelIndex, i + m_blockOffset);
}

GridItem *CGUIEPGGridContainer::GetPrevItem(int channel)
{
  int channelIndex = channel + m_channelOffset;
  int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex >= m_gridModel->ChannelItemsSize() || blockIndex >= m_gridModel->GetBlockCount())
    return nullptr;

  int i = m_blockCursor;

  while (i > 0 && m_gridModel->GetGridItem(channelIndex, i + m_blockOffset) == m_gridModel->GetGridItem(channelIndex, blockIndex))
    i--;

  return m_gridModel->GetGridItemPtr(channelIndex, i + m_blockOffset);
}

GridItem *CGUIEPGGridContainer::GetItem(int channel)
{
  int channelIndex = channel + m_channelOffset;
  int blockIndex = m_blockCursor + m_blockOffset;
  if (channelIndex >= m_gridModel->ChannelItemsSize() || blockIndex >= m_gridModel->GetBlockCount())
    return nullptr;

  return m_gridModel->GetGridItemPtr(channelIndex, blockIndex);
}

void CGUIEPGGridContainer::SetFocus(bool focus)
{
  if (focus != HasFocus())
    SetInvalid();

  CGUIControl::SetFocus(focus);
}

void CGUIEPGGridContainer::ScrollToChannelOffset(int offset)
{
  CSingleLock lock(m_critSection);

  float size = m_programmeLayout->Size(m_orientation);
  int range = m_channelsPerPage / 4;

  if (range <= 0)
    range = 1;

  if (offset * size < m_channelScrollOffset &&  m_channelScrollOffset - offset * size > size * range)
  {
    // scrolling up, and we're jumping more than 0.5 of a screen
    m_channelScrollOffset = (offset + range) * size;
  }

  if (offset * size > m_channelScrollOffset && offset * size - m_channelScrollOffset > size * range)
  {
    // scrolling down, and we're jumping more than 0.5 of a screen
    m_channelScrollOffset = (offset - range) * size;
  }

  m_channelScrollSpeed = (offset * size - m_channelScrollOffset) / m_scrollTime;
  m_channelOffset = offset;
  MarkDirtyRegion();
}

void CGUIEPGGridContainer::ScrollToBlockOffset(int offset)
{
  CSingleLock lock(m_critSection);

  // make sure offset is in valid range
  offset = std::max(0, std::min(offset, m_gridModel->GetBlockCount() - m_blocksPerPage));

  float size = m_blockSize;
  int range = m_blocksPerPage / 1;

  if (range <= 0)
    range = 1;

  if (offset * size < m_programmeScrollOffset &&  m_programmeScrollOffset - offset * size > size * range)
  {
    // scrolling left, and we're jumping more than 0.5 of a screen
    m_programmeScrollOffset = (offset + range) * size;
  }

  if (offset * size > m_programmeScrollOffset && offset * size - m_programmeScrollOffset > size * range)
  {
    // scrolling right, and we're jumping more than 0.5 of a screen
    m_programmeScrollOffset = (offset - range) * size;
  }

  m_programmeScrollSpeed = (offset * size - m_programmeScrollOffset) / m_scrollTime;
  m_blockOffset = offset;
  MarkDirtyRegion();
}

void CGUIEPGGridContainer::ValidateOffset()
{
  CSingleLock lock(m_critSection);

  if (!m_programmeLayout)
    return;

  float pos = (m_orientation == VERTICAL) ? m_channelHeight : m_channelWidth;

  if (m_gridModel->ChannelItemsSize() &&
      (m_channelOffset > m_gridModel->ChannelItemsSize() - m_channelsPerPage || m_channelScrollOffset > (m_gridModel->ChannelItemsSize() - m_channelsPerPage) * pos))
  {
    m_channelOffset = m_gridModel->ChannelItemsSize() - m_channelsPerPage;
    m_channelScrollOffset = m_channelOffset * pos;
  }

  if (m_channelOffset < 0 || m_channelScrollOffset < 0)
  {
    m_channelOffset = 0;
    m_channelScrollOffset = 0;
  }

  if (m_gridModel->GetBlockCount() &&
      (m_blockOffset > m_gridModel->GetBlockCount() - m_blocksPerPage || m_programmeScrollOffset > (m_gridModel->GetBlockCount() - m_blocksPerPage) * m_blockSize))
  {
    m_blockOffset = m_gridModel->GetBlockCount() - m_blocksPerPage;
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
  }

  if (m_blockOffset < 0 || m_programmeScrollOffset < 0)
  {
    m_blockOffset = 0;
    m_programmeScrollOffset = 0;
  }
}

void CGUIEPGGridContainer::LoadLayout(TiXmlElement *layout)
{
  /* layouts for the channel column */
  TiXmlElement *itemElement = layout->FirstChildElement("channellayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    m_channelLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("channellayout");
  }
  itemElement = layout->FirstChildElement("focusedchannellayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), true, m_width, m_height);
    m_focusedChannelLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedchannellayout");
  }

  /* layouts for the grid items */
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), true, m_width, m_height);
    m_focusedProgrammeLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
  itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    m_programmeLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }

  /* layout for the date label for the grid */
  itemElement = layout->FirstChildElement("rulerdatelayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    m_rulerDateLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("rulerdatelayout");
  }

  /* layout for the timeline for the grid */
  itemElement = layout->FirstChildElement("rulerlayout");
  while (itemElement)
  {
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    m_rulerLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("rulerlayout");
  }
}

std::string CGUIEPGGridContainer::GetDescription() const
{
  CSingleLock lock(m_critSection);

  std::string strLabel;
  int item = GetSelectedItem();
  if (item >= 0 && item < m_gridModel->ProgrammeItemsSize())
  {
    CGUIListItemPtr pItem(m_gridModel->GetProgrammeItem(item));
    strLabel = pItem->GetLabel();
  }
  return strLabel;
}

void CGUIEPGGridContainer::GoToBegin()
{
  ScrollToBlockOffset(0);
  SetBlock(0);
}

void CGUIEPGGridContainer::GoToEnd()
{
  int blocksEnd = 0;   // the end block of the last epg element for the selected channel
  int blocksStart = 0; // the start block of the last epg element for the selected channel
  int blockOffset = 0; // the block offset to scroll to
  for (int blockIndex = m_gridModel->GetBlockCount() - 1; blockIndex >= 0 && (!blocksEnd || !blocksStart); blockIndex--)
  {
    if (!blocksEnd && m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, blockIndex))
      blocksEnd = blockIndex;

    if (blocksEnd && m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, blocksEnd) != m_gridModel->GetGridItem(m_channelCursor + m_channelOffset, blockIndex))
      blocksStart = blockIndex + 1;
  }

  if (blocksEnd - blocksStart > m_blocksPerPage)
    blockOffset = blocksStart;
  else if (blocksEnd > m_blocksPerPage)
    blockOffset = blocksEnd - m_blocksPerPage;

  ScrollToBlockOffset(blockOffset); // scroll to the start point of the last epg element
  SetBlock(m_blocksPerPage - 1);    // select the last epg element
}

void CGUIEPGGridContainer::GoToNow()
{
  CDateTime currentDate = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
  int offset = (currentDate - m_gridModel->GetGridStart()).GetSecondsTotal() / 60 / CGUIEPGGridContainerModel::MINSPERBLOCK - GetPageNowOffset();
  ScrollToBlockOffset(offset);
  SetBlock(GetPageNowOffset());
}

void CGUIEPGGridContainer::SetTimelineItems(const std::unique_ptr<CFileItemList> &items, const CDateTime &gridStart, const CDateTime &gridEnd)
{
  int iRulerUnit;
  int iBlocksPerPage;
  float fBlockSize;
  {
    CSingleLock lock(m_critSection);

    UpdateLayout();
    iRulerUnit = m_rulerUnit;
    iBlocksPerPage = m_blocksPerPage;
    fBlockSize = m_blockSize;
  }

  std::unique_ptr<CGUIEPGGridContainerModel> oldOutdatedGridModel;
  std::unique_ptr<CGUIEPGGridContainerModel> oldUpdatedGridModel;
  std::unique_ptr<CGUIEPGGridContainerModel> newUpdatedGridModel(new CGUIEPGGridContainerModel);
  // can be very expensive. never call with lock acquired.
  newUpdatedGridModel->Refresh(items, gridStart, gridEnd, iRulerUnit, iBlocksPerPage, fBlockSize);

  {
    CSingleLock lock(m_critSection);

    // grid contains CFileItem instances. CFileItem dtor locks global graphics mutex.
    // by increasing its refcount make sure, old data are not deleted while we're holding own mutex.
    oldOutdatedGridModel = std::move(m_outdatedGridModel);
    oldUpdatedGridModel = std::move(m_updatedGridModel);

    m_updatedGridModel = std::move(newUpdatedGridModel);
  }
}

void CGUIEPGGridContainer::GoToChannel(int channelIndex)
{
  if (channelIndex < m_channelsPerPage)
  {
    // first page
    ScrollToChannelOffset(0);
    SetChannel(channelIndex);
  }
  else if (channelIndex > m_gridModel->ChannelItemsSize() - m_channelsPerPage)
  {
    // last page
    ScrollToChannelOffset(m_gridModel->ChannelItemsSize() - m_channelsPerPage);
    SetChannel(channelIndex - (m_gridModel->ChannelItemsSize() - m_channelsPerPage));
  }
  else
  {
    ScrollToChannelOffset(channelIndex - m_channelCursor);
    SetChannel(m_channelCursor);
  }
}

void CGUIEPGGridContainer::GoToBlock(int blockIndex)
{
  int lastPage = m_gridModel->GetBlockCount() - m_blocksPerPage;
  if (blockIndex > lastPage)
  {
    // last page
    ScrollToBlockOffset(lastPage);
    SetBlock(blockIndex - lastPage);
  }
  else
  {
    ScrollToBlockOffset(blockIndex - m_blockCursor);
    SetBlock(m_blockCursor);
  }
}

void CGUIEPGGridContainer::UpdateLayout()
{
  CGUIListItemLayout *oldFocusedChannelLayout   = m_focusedChannelLayout;
  CGUIListItemLayout *oldChannelLayout          = m_channelLayout;
  CGUIListItemLayout *oldFocusedProgrammeLayout = m_focusedProgrammeLayout;
  CGUIListItemLayout *oldProgrammeLayout        = m_programmeLayout;
  CGUIListItemLayout *oldRulerLayout            = m_rulerLayout;
  CGUIListItemLayout *oldRulerDateLayout        = m_rulerDateLayout;

  GetCurrentLayouts();

  // Note: m_rulerDateLayout is optional
  if (!m_focusedProgrammeLayout || !m_programmeLayout || !m_focusedChannelLayout || !m_channelLayout || !m_rulerLayout)
    return;

  if (oldChannelLayout == m_channelLayout && oldFocusedChannelLayout == m_focusedChannelLayout &&
      oldProgrammeLayout == m_programmeLayout && oldFocusedProgrammeLayout == m_focusedProgrammeLayout &&
      oldRulerLayout == m_rulerLayout && oldRulerDateLayout == m_rulerDateLayout)
    return; // nothing has changed, so don't update stuff

  m_channelHeight = m_channelLayout->Size(VERTICAL);
  m_channelWidth  = m_channelLayout->Size(HORIZONTAL);

  m_rulerDateHeight = m_rulerDateLayout ? m_rulerDateLayout->Size(VERTICAL) : 0;
  m_rulerDateWidth = m_rulerDateLayout ? m_rulerDateLayout->Size(HORIZONTAL) : 0;

  if (m_orientation == VERTICAL)
  {
    m_rulerHeight       = m_rulerLayout->Size(VERTICAL);
    m_gridPosX          = m_posX + m_channelWidth;
    m_gridPosY          = m_posY + m_rulerHeight + m_rulerDateHeight;
    m_gridWidth         = m_width - m_channelWidth;
    m_gridHeight        = m_height - m_rulerHeight - m_rulerDateHeight;
    m_blockSize         = m_gridWidth / m_blocksPerPage;
    m_rulerWidth        = m_rulerUnit * m_blockSize;
    m_channelPosX       = m_posX;
    m_channelPosY       = m_posY + m_rulerHeight + m_rulerDateHeight;
    m_rulerPosX         = m_posX + m_channelWidth;
    m_rulerPosY         = m_posY + m_rulerDateHeight;
    m_channelsPerPage   = m_gridHeight / m_channelHeight;
    m_programmesPerPage = (m_gridWidth / m_blockSize) + 1;

    m_programmeLayout->SetHeight(m_channelHeight);
    m_focusedProgrammeLayout->SetHeight(m_channelHeight);
  }
  else
  {
    m_rulerWidth        = m_rulerLayout->Size(HORIZONTAL);
    m_gridPosX          = m_posX + m_rulerWidth;
    m_gridPosY          = m_posY + m_channelHeight + m_rulerDateHeight;
    m_gridWidth         = m_width - m_rulerWidth;
    m_gridHeight        = m_height - m_channelHeight - m_rulerDateHeight;
    m_blockSize         = m_gridHeight / m_blocksPerPage;
    m_rulerHeight       = m_rulerUnit * m_blockSize;
    m_channelPosX       = m_posX + m_rulerWidth;
    m_channelPosY       = m_posY + m_rulerDateHeight;
    m_rulerPosX         = m_posX;
    m_rulerPosY         = m_posY + m_channelHeight + m_rulerDateHeight;
    m_channelsPerPage   = m_gridWidth / m_channelWidth;
    m_programmesPerPage = (m_gridHeight / m_blockSize) + 1;

    m_programmeLayout->SetWidth(m_channelWidth);
    m_focusedProgrammeLayout->SetWidth(m_channelWidth);
  }

  // ensure that the scroll offsets are a multiple of our sizes
  m_channelScrollOffset   = m_channelOffset * m_programmeLayout->Size(m_orientation);
  m_programmeScrollOffset = m_blockOffset * m_blockSize;
}

void CGUIEPGGridContainer::UpdateScrollOffset(unsigned int currentTime)
{
  if (!m_programmeLayout)
    return;

  m_channelScrollOffset += m_channelScrollSpeed * (currentTime - m_channelScrollLastTime);
  if ((m_channelScrollSpeed < 0 && m_channelScrollOffset < m_channelOffset * m_programmeLayout->Size(m_orientation)) ||
      (m_channelScrollSpeed > 0 && m_channelScrollOffset > m_channelOffset * m_programmeLayout->Size(m_orientation)))
  {
    m_channelScrollOffset = m_channelOffset * m_programmeLayout->Size(m_orientation);
    m_channelScrollSpeed = 0;
  }

  m_channelScrollLastTime = currentTime;
  m_programmeScrollOffset += m_programmeScrollSpeed * (currentTime - m_programmeScrollLastTime);

  if ((m_programmeScrollSpeed < 0 && m_programmeScrollOffset < m_blockOffset * m_blockSize) ||
      (m_programmeScrollSpeed > 0 && m_programmeScrollOffset > m_blockOffset * m_blockSize))
  {
    m_programmeScrollOffset = m_blockOffset * m_blockSize;
    m_programmeScrollSpeed = 0;
  }

  m_programmeScrollLastTime = currentTime;

  if (m_channelScrollSpeed || m_programmeScrollSpeed)
    MarkDirtyRegion();
}

void CGUIEPGGridContainer::GetCurrentLayouts()
{
  m_channelLayout = nullptr;

  for (unsigned int i = 0; i < m_channelLayouts.size(); i++)
  {
    if (m_channelLayouts[i].CheckCondition())
    {
      m_channelLayout = &m_channelLayouts[i];
      break;
    }
  }

  if (!m_channelLayout && !m_channelLayouts.empty())
    m_channelLayout = &m_channelLayouts[0];  // failsafe

  m_focusedChannelLayout = nullptr;

  for (unsigned int i = 0; i < m_focusedChannelLayouts.size(); i++)
  {
    if (m_focusedChannelLayouts[i].CheckCondition())
    {
      m_focusedChannelLayout = &m_focusedChannelLayouts[i];
      break;
    }
  }

  if (!m_focusedChannelLayout && !m_focusedChannelLayouts.empty())
    m_focusedChannelLayout = &m_focusedChannelLayouts[0];  // failsafe

  m_programmeLayout = nullptr;

  for (unsigned int i = 0; i < m_programmeLayouts.size(); i++)
  {
    if (m_programmeLayouts[i].CheckCondition())
    {
      m_programmeLayout = &m_programmeLayouts[i];
      break;
    }
  }

  if (!m_programmeLayout && !m_programmeLayouts.empty())
    m_programmeLayout = &m_programmeLayouts[0];  // failsafe

  m_focusedProgrammeLayout = nullptr;

  for (unsigned int i = 0; i < m_focusedProgrammeLayouts.size(); i++)
  {
    if (m_focusedProgrammeLayouts[i].CheckCondition())
    {
      m_focusedProgrammeLayout = &m_focusedProgrammeLayouts[i];
      break;
    }
  }

  if (!m_focusedProgrammeLayout && !m_focusedProgrammeLayouts.empty())
    m_focusedProgrammeLayout = &m_focusedProgrammeLayouts[0];  // failsafe

  m_rulerLayout = nullptr;

  for (unsigned int i = 0; i < m_rulerLayouts.size(); i++)
  {
    if (m_rulerLayouts[i].CheckCondition())
    {
      m_rulerLayout = &m_rulerLayouts[i];
      break;
    }
  }

  if (!m_rulerLayout && !m_rulerLayouts.empty())
    m_rulerLayout = &m_rulerLayouts[0];  // failsafe

  m_rulerDateLayout = nullptr;

  for (unsigned int i = 0; i < m_rulerDateLayouts.size(); i++)
  {
    if (m_rulerDateLayouts[i].CheckCondition())
    {
      m_rulerDateLayout = &m_rulerDateLayouts[i];
      break;
    }
  }

  // Note: m_rulerDateLayout is optional; so no "failsafe" logic here (see above)
}

void CGUIEPGGridContainer::SetRenderOffset(const CPoint &offset)
{
  m_renderOffset = offset;
}

void CGUIEPGGridContainer::GetChannelCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_channelScrollSpeed > 0)
  {
    cacheBefore = 0;
    cacheAfter = m_cacheChannelItems;
  }
  else if (m_channelScrollSpeed < 0)
  {
    cacheBefore = m_cacheChannelItems;
    cacheAfter = 0;
  }
  else
  {
    cacheBefore = m_cacheChannelItems / 2;
    cacheAfter = m_cacheChannelItems / 2;
  }
}

void CGUIEPGGridContainer::GetProgrammeCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_programmeScrollSpeed > 0)
  {
    cacheBefore = 0;
    cacheAfter = m_cacheProgrammeItems;
  }
  else if (m_programmeScrollSpeed < 0)
  {
    cacheBefore = m_cacheProgrammeItems;
    cacheAfter = 0;
  }
  else
  {
    cacheBefore = m_cacheProgrammeItems / 2;
    cacheAfter = m_cacheProgrammeItems / 2;
  }
}

void CGUIEPGGridContainer::HandleChannels(bool bRender, unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_focusedChannelLayout || !m_channelLayout)
    return;

  int chanOffset = MathUtils::round_int(m_channelScrollOffset / m_programmeLayout->Size(m_orientation));

  int cacheBeforeChannel, cacheAfterChannel;
  GetChannelCacheOffsets(cacheBeforeChannel, cacheAfterChannel);

  if (bRender)
  {
    if (m_orientation == VERTICAL)
      g_graphicsContext.SetClipRegion(m_channelPosX, m_channelPosY, m_channelWidth, m_gridHeight);
    else
      g_graphicsContext.SetClipRegion(m_channelPosX, m_channelPosY, m_gridWidth, m_channelHeight);
  }
  else
  {
    // Free memory not used on screen
    if (m_gridModel->ChannelItemsSize() > m_channelsPerPage + cacheBeforeChannel + cacheAfterChannel)
      m_gridModel->FreeChannelMemory(chanOffset - cacheBeforeChannel, chanOffset + m_channelsPerPage + 1 + cacheAfterChannel);
  }

  CPoint originChannel = CPoint(m_channelPosX, m_channelPosY) + m_renderOffset;
  float pos;
  float end;

  if (m_orientation == VERTICAL)
  {
    pos = originChannel.y;
    end = m_posY + m_height;
  }
  else
  {
    pos = originChannel.x;
    end = m_posX + m_width;
  }

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (chanOffset - cacheBeforeChannel) * m_channelLayout->Size(m_orientation) - m_channelScrollOffset;
  if (m_channelOffset + m_channelCursor < chanOffset)
    drawOffset += m_focusedChannelLayout->Size(m_orientation) - m_channelLayout->Size(m_orientation);

  pos += drawOffset;
  end += cacheAfterChannel * m_channelLayout->Size(m_orientation);

  float focusedPos = 0;
  CGUIListItemPtr focusedItem;

  CFileItemPtr item;
  int current = chanOffset;
  while (pos < end && m_gridModel->HasChannelItems())
  {
    int itemNo = current;
    if (itemNo >= m_gridModel->ChannelItemsSize())
      break;

    bool focused = (current == m_channelOffset + m_channelCursor);
    if (itemNo >= 0)
    {
      item = m_gridModel->GetChannelItem(itemNo);
      if (bRender)
      {
        // render our item
        if (focused)
        {
          focusedPos = pos;
          focusedItem = item;
        }
        else
        {
          if (m_orientation == VERTICAL)
            RenderItem(originChannel.x, pos, item.get(), false);
          else
            RenderItem(pos, originChannel.y, item.get(), false);
        }
      }
      else
      {
        // process our item
        if (m_orientation == VERTICAL)
          ProcessItem(originChannel.x, pos, item, m_lastItem, focused, m_channelLayout, m_focusedChannelLayout, currentTime, dirtyregions);
        else
          ProcessItem(pos, originChannel.y, item, m_lastItem, focused, m_channelLayout, m_focusedChannelLayout, currentTime, dirtyregions);
      }
    }
    // increment our position
    pos += focused ? m_focusedChannelLayout->Size(m_orientation) : m_channelLayout->Size(m_orientation);
    current++;
  }

  if (bRender)
  {
    // render focused item last so it can overlap other items
    if (focusedItem)
    {
      if (m_orientation == VERTICAL)
        RenderItem(originChannel.x, focusedPos, focusedItem.get(), true);
      else
        RenderItem(focusedPos, originChannel.y, focusedItem.get(), true);
    }

    g_graphicsContext.RestoreClipRegion();
  }
}

void CGUIEPGGridContainer::HandleRulerDate(bool bRender, unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_rulerDateLayout || m_gridModel->RulerItemsSize() <= 1 || m_gridModel->IsZeroGridDuration())
    return;

  CFileItemPtr item(m_gridModel->GetRulerItem(0));

  if (bRender)
  {
    // Render single ruler item with date of selected programme
    g_graphicsContext.SetClipRegion(m_posX, m_posY, m_rulerDateWidth, m_rulerDateHeight);
    RenderItem(m_posX, m_posY, item.get(), false);
    g_graphicsContext.RestoreClipRegion();
  }
  else
  {
    int rulerOffset = MathUtils::round_int(m_programmeScrollOffset / m_blockSize);
    item->SetLabel(m_gridModel->GetRulerItem(rulerOffset / m_rulerUnit + 1)->GetLabel2());

    CFileItemPtr lastitem;
    ProcessItem(m_posX, m_posY, item, lastitem, false, m_rulerDateLayout, m_rulerDateLayout, currentTime, dirtyregions);
  }
}

void CGUIEPGGridContainer::HandleRuler(bool bRender, unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_rulerLayout || m_gridModel->RulerItemsSize() <= 1 || m_gridModel->IsZeroGridDuration())
    return;

  int rulerOffset = MathUtils::round_int(m_programmeScrollOffset / m_blockSize);

  CFileItemPtr item(m_gridModel->GetRulerItem(0));
  CFileItemPtr lastitem;
  int cacheBeforeRuler, cacheAfterRuler;

  if (bRender)
  {
    if (!m_rulerDateLayout)
    {
      // Render single ruler item with date of selected programme
      g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
      RenderItem(m_posX, m_posY, item.get(), false);
      g_graphicsContext.RestoreClipRegion();
    }

    // render ruler items
    GetProgrammeCacheOffsets(cacheBeforeRuler, cacheAfterRuler);

    if (m_orientation == VERTICAL)
      g_graphicsContext.SetClipRegion(m_rulerPosX, m_rulerPosY, m_gridWidth, m_rulerHeight);
    else
      g_graphicsContext.SetClipRegion(m_rulerPosX, m_rulerPosY, m_rulerWidth, m_gridHeight);
  }
  else
  {
    if (!m_rulerDateLayout)
    {
      item->SetLabel(m_gridModel->GetRulerItem(rulerOffset / m_rulerUnit + 1)->GetLabel2());
      ProcessItem(m_posX, m_posY, item, lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_channelWidth);
    }

    GetProgrammeCacheOffsets(cacheBeforeRuler, cacheAfterRuler);

    // Free memory not used on screen
    if (m_gridModel->RulerItemsSize() > m_blocksPerPage + cacheBeforeRuler + cacheAfterRuler)
      m_gridModel->FreeRulerMemory(rulerOffset / m_rulerUnit + 1 - cacheBeforeRuler, rulerOffset / m_rulerUnit + 1 + m_blocksPerPage + 1 + cacheAfterRuler);
  }

  CPoint originRuler = CPoint(m_rulerPosX, m_rulerPosY) + m_renderOffset;
  float pos;
  float end;

  if (m_orientation == VERTICAL)
  {
    pos = originRuler.x;
    end = m_posX + m_width;
  }
  else
  {
    pos = originRuler.y;
    end = m_posY + m_height;
  }

  float drawOffset = (rulerOffset - cacheBeforeRuler) * m_blockSize - m_programmeScrollOffset;
  pos += drawOffset;
  end += cacheAfterRuler * m_rulerLayout->Size(m_orientation == VERTICAL ? HORIZONTAL : VERTICAL);

  if (rulerOffset % m_rulerUnit != 0)
  {
    /* first ruler marker starts before current view */
    int startBlock = rulerOffset - 1;

    while (startBlock % m_rulerUnit != 0)
      startBlock--;

    int missingSection = rulerOffset - startBlock;

    pos -= missingSection * m_blockSize;
  }

  while (pos < end && (rulerOffset / m_rulerUnit + 1) < m_gridModel->RulerItemsSize())
  {
    item = m_gridModel->GetRulerItem(rulerOffset / m_rulerUnit + 1);

    if (m_orientation == VERTICAL)
    {
      if (bRender)
        RenderItem(pos, originRuler.y, item.get(), false);
      else
        ProcessItem(pos, originRuler.y, item, lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_rulerWidth);

      pos += m_rulerWidth;
    }
    else
    {
      if (bRender)
        RenderItem(originRuler.x, pos, item.get(), false);
      else
        ProcessItem(originRuler.x, pos, item, lastitem, false, m_rulerLayout, m_rulerLayout, currentTime, dirtyregions, m_rulerHeight);

      pos += m_rulerHeight;
    }

    rulerOffset += m_rulerUnit;
  }

  if (bRender)
    g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::HandleProgrammeGrid(bool bRender, unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (!m_focusedProgrammeLayout || !m_programmeLayout || m_gridModel->RulerItemsSize() <= 1 || m_gridModel->IsZeroGridDuration())
    return;

  int blockOffset = MathUtils::round_int(m_programmeScrollOffset / m_blockSize);
  int chanOffset  = MathUtils::round_int(m_channelScrollOffset / m_programmeLayout->Size(m_orientation));

  int cacheBeforeProgramme, cacheAfterProgramme;
  GetProgrammeCacheOffsets(cacheBeforeProgramme, cacheAfterProgramme);

  if (bRender)
    g_graphicsContext.SetClipRegion(m_gridPosX, m_gridPosY, m_gridWidth, m_gridHeight);

  CPoint originProgramme = CPoint(m_gridPosX, m_gridPosY) + m_renderOffset;
  float posA;
  float endA;
  float posB;
  float endB;

  if (m_orientation == VERTICAL)
  {
    posA = originProgramme.x;
    endA = m_posX + m_width;
    posB = originProgramme.y;
    endB = m_gridPosY + m_gridHeight;
  }
  else
  {
    posA = originProgramme.y;
    endA = m_posY + m_height;
    posB = originProgramme.x;
    endB = m_gridPosX + m_gridWidth;
  }

  endA += cacheAfterProgramme * m_blockSize;

  float drawOffsetA = blockOffset * m_blockSize - m_programmeScrollOffset;
  posA += drawOffsetA;
  float drawOffsetB = (chanOffset - cacheBeforeProgramme) * m_channelLayout->Size(m_orientation) - m_channelScrollOffset;
  posB += drawOffsetB;

  int channel = chanOffset;

  float focusedPosX = 0;
  float focusedPosY = 0;
  CFileItemPtr focusedItem;
  CFileItemPtr item;

  while (posB < endB && m_gridModel->HasChannelItems())
  {
    if (channel >= m_gridModel->ChannelItemsSize())
      break;

    if (!bRender)
    {
      // Free memory not used on screen
      m_gridModel->FreeProgrammeMemory(channel, blockOffset - cacheBeforeProgramme, blockOffset + m_programmesPerPage + 1 + cacheAfterProgramme);
    }

    int block = blockOffset;
    float posA2 = posA;

    item = m_gridModel->GetGridItem(channel, block);

    if (blockOffset > 0 && item == m_gridModel->GetGridItem(channel, blockOffset - 1))
    {
      /* first program starts before current view */
      int startBlock = blockOffset - 1;

      while (startBlock >= 0 && m_gridModel->GetGridItem(channel, startBlock) == item)
        startBlock--;

      block = startBlock + 1;
      int missingSection = blockOffset - block;
      posA2 -= missingSection * m_blockSize;
    }

    while (posA2 < endA && m_gridModel->HasProgrammeItems())
    {
      if (block >= m_gridModel->GetBlockCount())
        break;

      item = m_gridModel->GetGridItem(channel, block);

      if (!item || !item.get()->IsFileItem())
        break;

      bool focused = (channel == m_channelOffset + m_channelCursor) && (item == m_gridModel->GetGridItem(m_channelOffset + m_channelCursor, m_blockOffset + m_blockCursor));

      if (bRender) //! @todo Why the functional difference wrt truncate here?
      {
        // reset to grid start position if first item is out of grid view
        if (posA2 < posA)
          posA2 = posA;

        // render our item
        if (focused)
        {
          focusedPosX = posA2;
          focusedPosY = posB;
          focusedItem = item;
        }
        else
        {
          if (m_orientation == VERTICAL)
            RenderItem(posA2, posB, item.get(), focused);
          else
            RenderItem(posB, posA2, item.get(), focused);
        }
      }
      else
      {
        // calculate the size to truncate if item is out of grid view
        float truncateSize = 0;
        if (posA2 < posA)
        {
          truncateSize = posA - posA2;
          posA2 = posA; // reset to grid start position
        }

        {
          CSingleLock lock(m_critSection);
          // truncate item's width
          m_gridModel->SetGridItemWidth(channel, block, m_gridModel->GetGridItemOriginWidth(channel, block) - truncateSize);
        }

        if (m_orientation == VERTICAL)
          ProcessItem(posA2, posB, item, m_lastChannel, focused, m_programmeLayout, m_focusedProgrammeLayout, currentTime, dirtyregions, m_gridModel->GetGridItemWidth(channel, block));
        else
          ProcessItem(posB, posA2, item, m_lastChannel, focused, m_programmeLayout, m_focusedProgrammeLayout, currentTime, dirtyregions, m_gridModel->GetGridItemWidth(channel, block));
      }

      // increment our X position
      posA2 += m_gridModel->GetGridItemWidth(channel, block); // assumes focused & unfocused layouts have equal length
      block += MathUtils::round_int(m_gridModel->GetGridItemOriginWidth(channel, block) / m_blockSize);
    }

    // increment our Y position
    channel++;
    posB += (m_orientation == VERTICAL) ? m_channelHeight : m_channelWidth;
  }

  if (bRender)
  {
    // and render the focused item last (for overlapping purposes)
    if (focusedItem)
    {
      if (m_orientation == VERTICAL)
        RenderItem(focusedPosX, focusedPosY, focusedItem.get(), true);
      else
        RenderItem(focusedPosY, focusedPosX, focusedItem.get(), true);
    }

    g_graphicsContext.RestoreClipRegion();
  }
}