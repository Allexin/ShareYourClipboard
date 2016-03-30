/*
 * ShareYourClipboard - cross-platform clipboard share tool
 * Copyright (C) 2015-2016  Alexander Basov <basovav@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QMainWindow>
#include "../data/cclipboardmanager.h"

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QMainWindow
{
    Q_OBJECT
protected:
    cClipboardManager*  m_ClipboardManager;
    void loadPreferences();
    void applyPreferences();
public:
    explicit SettingsWindow(cClipboardManager* ClipboardManager);
    ~SettingsWindow();

private:
    Ui::SettingsWindow *ui;
protected:
    virtual void showEvent(QShowEvent * event) override;
signals:
    void preferencesChange();
public slots:
    void handleButtonApply();
    void handleButtonCancel();
    void handleButtonGenerateSecretKey();
};

#endif // SETTINGSWINDOW_H
