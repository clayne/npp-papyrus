/*
This file is part of Papyrus Plugin for Notepad++.

Copyright (C) 2022 blu3mania <blu3mania@hotmail.com>

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
*/

#include "MultiTabbedDialog.hpp"

#include <stdexcept>

#include <commctrl.h>

namespace papyrus {

  MultiTabbedDialog::~MultiTabbedDialog() {
    for (auto const& tabItem : tabItems) {
      HWND handle = tabItem.second.handle;
      if (handle != nullptr) {
        ::DestroyWindow(handle);
      }
    }
  }

  bool MultiTabbedDialog::removeTab(tab_id_t tab, bool destroy) {
    auto iter = findTab(tab);
    if (iter != tabs.end()) {
      // Remove the tab from tabs control
      ::SendDlgItemMessage(getHSelf(), tabsControlID, TCM_DELETEITEM, getTabIndex(iter), 0);

      auto iterItem = tabItems.find(tab);
      if (destroy) {
        tabs.erase(iter);
        ::DestroyWindow(iterItem->second.handle);
        tabItems.erase(iterItem);
        onTabDialogDestroyed(tab);
      } else {
        // Move the tab to hidden tabs list so it can be added back later
        hiddenTabs.splice(hiddenTabs.end(), tabs, iter);
      }
      return true;
    } else {
      return false;
    }
  }

  void MultiTabbedDialog::showTab(tab_id_t tab) {
    if (currentTab != tab) {
      if (currentTab >= 0 ) {
        setTabVisibility(currentTab, false);
      }

      currentTab = tab;
      if (!isTabDialogCreated(currentTab)) {
        createTabDialog(currentTab);
      }
      setTabVisibility(currentTab, true);
      ::SendDlgItemMessage(getHSelf(), tabsControlID, TCM_SETCURSEL, getTabIndex(currentTab), 0);
    }
  }

  // Protected methods
  //

  void MultiTabbedDialog::initControls() {
    // Calculate tab dialog's position:
    // 1. Find tabs control's position
    // 2. Add tabs control's height to get tab dialog's top position, so tabs are not overlapped by tab dialogs
    // 3. Add an 1-pixel margin to all 4 sides to make window border also visible
    ::GetWindowRect(DialogBase::getControl(tabsControlID), &tabDialogRect);
    POINT offset {};
    ::ScreenToClient(getHSelf(), &offset);
    ::OffsetRect(&tabDialogRect, offset.x, offset.y);

    tabDialogRect.top += 21;
    tabDialogRect.bottom--;
    tabDialogRect.left++;
    tabDialogRect.right--;
  }

  INT_PTR MultiTabbedDialog::handleNotifyMessage(WPARAM wParam, LPARAM lParam) {
    auto nmhdr = *(reinterpret_cast<LPNMHDR>(lParam));
    if (nmhdr.idFrom == tabsControlID) {
      if (nmhdr.code == TCN_SELCHANGE) {
        int tabIndex = static_cast<int>(::SendDlgItemMessage(getHSelf(), tabsControlID, TCM_GETCURSEL, 0, 0));
        showTab((std::next(tabs.begin(), tabIndex))->tab);
      }
      return TRUE;
    }

    return DialogBase::handleNotifyMessage(wParam, lParam);
  }

  // Private methods
  //

  INT_PTR CALLBACK MultiTabbedDialog::tabDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
      case WM_INITDIALOG: {
        // Store initialization data (pointer to Tab structure that contains tab info, including MultiTabbedDialog instance)
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
        return TRUE;
      }

      case WM_COMMAND: {
        Tab* tabInfo = reinterpret_cast<Tab*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (tabInfo == nullptr) {
          return FALSE;
        }
        return tabInfo->multiTabbedDialog->handleTabCommandMessage(tabInfo->tab, wParam, lParam);
      }
    }

    return FALSE;
  }

  void MultiTabbedDialog::createTabDialog(tab_id_t tab) {
    auto iter = findTab(tab);
    if (iter != tabs.end()) {
      tabItems[tab].handle = ::CreateDialogParam(getHinst(), MAKEINTRESOURCE(tabItems[tab].dialogID), getHSelf(), tabDialogProc, reinterpret_cast<LPARAM>(&(*iter)));
      ::SetWindowPos(tabItems[tab].handle, 0, tabDialogRect.left, tabDialogRect.top, tabDialogRect.right - tabDialogRect.left, tabDialogRect.bottom - tabDialogRect.top, SWP_HIDEWINDOW);
      onTabDialogCreated(tab);
    }
  }

  void MultiTabbedDialog::addTabAt(tab_id_t tab, int dialogID, std::wstring text, tab_list_t::const_iterator pos, bool lazyInitialization) {
    int tabIndex = getTabIndex(pos);

    // Check if tab already exists
    bool foundExistingTab = false;
    auto list = tabs;
    auto iter = findTab(tab);
    if (iter != tabs.end()) {
      foundExistingTab = true;
    } else {
      // Check hidden tabs as well
      iter = std::find_if(hiddenTabs.begin(), hiddenTabs.end(),
        [&](const auto& tabInfo) {
          return tabInfo.tab == tab;
        }
      );
      if (iter != hiddenTabs.end()) {
        list = hiddenTabs;
        foundExistingTab = true;
      }
    }

    if (foundExistingTab) {
      if (tabItems[tab].dialogID != dialogID) {
        throw std::invalid_argument("Cannot use the same tab ID for different dialogs");
      }
      tabs.splice(pos, list, iter);
    } else {
      tabs.insert(pos, Tab {
        .tab = tab,
        .multiTabbedDialog = this
      });

      tabItems[tab] = TabItem {
        .dialogID = dialogID
      };

      if (!lazyInitialization) {
        createTabDialog(tab);
      }
    }

    // Add tab to tabs control
    auto textCStr = text.c_str();
    TCITEM item {
      .mask = TCIF_TEXT,
      .pszText = const_cast<LPWSTR>(textCStr),
      .cchTextMax = static_cast<int>(_tcslen(textCStr))
    };
    ::SendDlgItemMessage(getHSelf(), tabsControlID, TCM_INSERTITEM, tabIndex, reinterpret_cast<LPARAM>(&item));
  }

} // namespace
