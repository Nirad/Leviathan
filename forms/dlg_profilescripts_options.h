#ifndef PROFILE_OPTIONS_H
#define PROFILE_OPTIONS_H

#include <QDialog>
#include <QModelIndex>
#include "../spherescript/scriptsprofile.h"
//#include <map>
#include <vector>


class QDirModel;
class QStandardItemModel;
class CheckableProxyModel;
class ScriptObj;

namespace Ui {
class Dlg_ProfileScripts_Options;
}

class Dlg_ProfileScripts_Options : public QDialog
{
    Q_OBJECT

public:
    explicit Dlg_ProfileScripts_Options(QWidget *parent = 0);
    ~Dlg_ProfileScripts_Options();

private slots:
    void on_listView_profiles_clicked(const QModelIndex &index);
    void on_lineEdit_editPath_textChanged(const QString &arg1);
    void on_pushButton_pathBrowse_clicked();
    void on_pushButton_profileAdd_clicked();
    void on_pushButton_profileSave_clicked();
    void on_pushButton_profileDelete_clicked();
    void on_pushButton_SelectAllScripts_clicked();
    void on_pushButton_clearSelection_clicked();

private:
    Ui::Dlg_ProfileScripts_Options *ui;
    QDirModel *m_scripts_model_base;
    CheckableProxyModel *m_scripts_model;
    QStandardItemModel *m_profiles_model;

    void updateProfilesView();
    bool checkScriptsFromProfile_loop(const std::string scriptFromProfile, const QModelIndex &proxyParent);
    void checkScriptsFromProfile(const ScriptsProfile *sp, const QModelIndex &proxyParent);
    void updateScriptsView(QString path);
    void saveProfilesToJson();

    std::vector<ScriptsProfile> m_scriptsProfiles;
    //std::map<QStandardItem*, int> m_profilesMap;    // links the item in the profiles list to the number of
    int m_currentProfileIndex = -1;
};


#endif // PROFILE_OPTIONS_H