/*
 * FBReader -- electronic book reader
 * Copyright (C) 2004-2007 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005 Mikhail Sobolev <mss@mawhrin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <map>

#include <ZLFile.h>
#include <ZLDir.h>
#include <ZLApplication.h>
#include <ZLDialogManager.h>

#include "FBFileHandler.h"
#include "../formats/FormatPlugin.h"

FBFileHandler::FBFileHandler() :
	DirectoryOption(ZLOption::LOOK_AND_FEEL_CATEGORY, "OpenFileDialog", "Directory", ZLApplication::HomeDirectory), myIsUpToDate(false), mySelectedIndex(0) {
	myDir = ZLFile(DirectoryOption.value()).directory();
	if (myDir.isNull()) {
		myDir = ZLFile(ZLApplication::HomeDirectory).directory();
	}
}

FBFileHandler::~FBFileHandler() {
	DirectoryOption.setValue(myDir->name());
}

/*
std::string FBFileHandler::relativeId(const ZLTreeNode &node) const {
	return (node.id() == "..") ? myDir->shortName() : "..";
}
*/

void FBFileHandler::changeFolder(const ZLTreeNode &node) {
	shared_ptr<ZLDir> dir = ZLFile(myDir->itemName(node.id())).directory();
	if (!dir.isNull()) {
		const std::string selectedId = myDir->shortName();
		myDir = dir;
		myIsUpToDate = false;
		mySubnodes.clear();
		mySelectedIndex = 0;
		if (node.id() == "..") {
			int index = 0;
			const std::vector<ZLTreeNodePtr> &subnodes = this->subnodes();
			for (std::vector<ZLTreeNodePtr>::const_iterator it = subnodes.begin(); it != subnodes.end(); ++it) {
				if ((*it)->id() == selectedId) {
					mySelectedIndex = index;
					break;
				}
				++index;
			}
		}
		addUpdateInfo(UPDATE_ALL);
	}
}

const std::string FBFileHandler::stateDisplayName() const {
	return ZLFile(myDir->name()).utf8Path();
}

const std::vector<ZLTreeNodePtr> &FBFileHandler::subnodes() const {
	static const std::string FolderIcon = "folder";
	static const std::string ZipFolderIcon = "zipfolder";
	static std::map<FormatPlugin*,std::string> PluginIcons;

	if (!myIsUpToDate) {
		if (myDir->name() != ZLApplication::FileNameDelimiter) {
			mySubnodes.push_back(new ZLTreeNode("..", "..", FolderIcon, true));
		}

		std::map<std::string,ZLTreeNodePtr> folderNodes;
		std::map<std::string,ZLTreeNodePtr> fileNodes;

		std::vector<std::string> names;
		std::vector<std::string>::const_iterator it;

		myDir->collectSubDirs(names, true);
		for (it = names.begin(); it != names.end(); ++it) {
			const std::string displayName = ZLFile(*it).utf8FullName();
			folderNodes[displayName] = new ZLTreeNode(*it, displayName, FolderIcon, true);
		}
		names.clear();

		myDir->collectFiles(names, true);
		for (it = names.begin(); it != names.end(); ++it) {
			if (!it->empty()) {
				ZLFile file(myDir->itemName(*it));
				FormatPlugin *plugin = PluginCollection::instance().plugin(file, false);
				if (plugin != 0) {
					std::string icon = PluginIcons[plugin];
					if (icon.empty()) {
						icon = plugin->iconName();
						PluginIcons[plugin] = icon;
					}
					const std::string displayName = file.utf8FullName();
					fileNodes[displayName] = new ZLTreeNode(*it, displayName, icon, false);
				} else if (file.isArchive()) {
					const std::string displayName = file.utf8FullName();
					folderNodes[displayName] = new ZLTreeNode(*it, displayName, ZipFolderIcon, true);
				}
			}
		}

		std::map<std::string,ZLTreeNodePtr>::const_iterator jt;
		for (jt = folderNodes.begin(); jt != folderNodes.end(); ++jt) {
			mySubnodes.push_back(jt->second);
		}
		for (jt = fileNodes.begin(); jt != fileNodes.end(); ++jt) {
			mySubnodes.push_back(jt->second);
		}
		myIsUpToDate = true;
	}
	return mySubnodes;
}

int FBFileHandler::selectedIndex() const {
	return mySelectedIndex;
}

bool FBFileHandler::accept(const ZLTreeNode &node) {
	const std::string name = myDir->itemName(node.id());
	FormatPlugin *plugin = PluginCollection::instance().plugin(ZLFile(name), false);
	const std::string message = (plugin == 0) ? "Unknown File Format" : plugin->tryOpen(name);
	if (!message.empty()) {
		ZLDialogManager::instance().errorBox("Error", "Couldn't Open:\n" + message);
		return false;
	}

	myDescription = BookDescription::getDescription(name);
	return true;
}
