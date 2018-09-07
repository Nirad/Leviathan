#include "maintab_chars.h"
#include "ui_maintab_chars.h"

#include <QMessageBox>
#include <QStandardItem>
#include <QGraphicsPixmapItem>
#include <QKeyEvent>

#include "globals.h"
#include "subdlg_searchobj.h"
#include "subdlg_spawn.h"
#include "../spherescript/scriptobjects.h"
#include "../spherescript/scriptutils.h"
#include "../uoclientfiles/uoanim.h"
#include "../keystrokesender/keystrokesender.h"
#include "cpputils/maps.h"


MainTab_Chars::MainTab_Chars(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainTab_Chars)
{
    ui->setupUi(this);

    installEventFilter(this);   // catch the keystrokes

    m_scriptSearch = nullptr;

    // Create the model for the organizer tree view
    m_organizer_model = new QStandardItemModel(0,0);
    ui->treeView_organizer->setModel(m_organizer_model);
    connect(ui->treeView_organizer->selectionModel(), SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),
            this, SLOT(onManual_treeView_organizer_selectionChanged(const QModelIndex&,const QModelIndex&)));

    // Create the model for the charList tree view
    m_objList_model = new QStandardItemModel(0,2);

    QStandardItem *charList_header0 = new QStandardItem("NPC Description");
    QStandardItem *charList_header1 = new QStandardItem("NPC ID");
    m_objList_model->setHorizontalHeaderItem(0, charList_header0);
    m_objList_model->setHorizontalHeaderItem(1, charList_header1);

    ui->treeView_objList->setModel(m_objList_model);
    connect(ui->treeView_objList->selectionModel(), SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),
            this, SLOT(onManual_treeView_objList_selectionChanged(const QModelIndex&,const QModelIndex&)));

    // Center column headers text
    ui->treeView_objList->header()->setDefaultAlignment(Qt::AlignHCenter);
    // Stretch column headers
    ui->treeView_objList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeView_objList->header()->setSectionResizeMode(1, QHeaderView::Interactive);

    // Set stretch factors to the splitter between the QGraphicsView and the lists layout
    ui->splitter_img_lists->setStretchFactor(0,3);  // column 0: lists layout
    ui->splitter_img_lists->setStretchFactor(1,1);  // column 1: QGraphicsView

    m_subdlg_searchObj = std::make_unique<SubDlg_SearchObj>(window());
}


MainTab_Chars::~MainTab_Chars()
{
    delete ui;

    delete m_organizer_model;
    delete m_objList_model;
}

bool MainTab_Chars::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return QObject::eventFilter(watched, event);

    QKeyEvent* keyEv = dynamic_cast<QKeyEvent*>(event);
    if (!keyEv)
        return false;
    if ( (keyEv->key()==Qt::Key_F2) || (keyEv->key()==Qt::Key_F3) )
    {   // pressed F1 or F2
        if (g_loadedScriptsProfile == -1)   // no profile loaded
            return false;
        if (m_scriptSearch == nullptr)      // i haven't set the search parameters
            return false;

        if (keyEv->key()==Qt::Key_F3)   // search forwards
            doSearch(false);
        else                            // search backwards
            doSearch(true);

        return false;
    }
    if ( (keyEv->key()==Qt::Key_F) && (keyEv->modifiers() & Qt::ControlModifier) )
    {   // presset CTRL + F
        on_pushButton_search_clicked();
        return false;
    }
    return QObject::eventFilter(watched, event);
}

void MainTab_Chars::updateViews()
{
    // Populate the Model to be applied to the QTreeView
    m_organizer_model->removeRows(0, m_organizer_model->rowCount());
    m_categoryMap.clear();
    m_subsectionMap.clear();
    m_objList_model->removeRows(0, m_objList_model->rowCount());
    m_objMapQItemToScript.clear();
    m_objMapScriptToQItem.clear();

	// disable redrawing the view for every item inserted, so that it will update only once, after all the insertions
    ui->treeView_organizer->setUpdatesEnabled(false);
    ui->treeView_objList->setUpdatesEnabled(false);

    QStandardItem *root = m_organizer_model->invisibleRootItem();

    const ScriptObjTree *trees[2] = { g_scriptObjTree_Chars, g_scriptObjTree_Spawns };
    for (int tree_i = 0; tree_i < 2; ++tree_i)
    {
        for (ScriptCategory* categoryInst : trees[tree_i]->m_categories)
        {
            QStandardItem *categoryItem = new QStandardItem(categoryInst->m_categoryName.c_str());
            m_categoryMap[categoryItem] = categoryInst;
            categoryItem->setSelectable(false);
            if (tree_i == 1)
                categoryItem->setForeground(QBrush(QColor("red")));
            root->appendRow(categoryItem);
            for (ScriptSubsection* subsectionInst : categoryInst->m_subsections)
            {
                QStandardItem *subsectionItem = new QStandardItem(subsectionInst->m_subsectionName.c_str());
                categoryItem->appendRow(subsectionItem);
                m_subsectionMap[subsectionItem] = subsectionInst;    // So that i know at which ScriptSubsection each QStandardItem corresponds.
            }
        }
    }

    //m_organizer_model->sort(0, Qt::AscendingOrder);   // Order alphabetically. -> not needed anymore, since the whole ScriptObjTree is now sorted after the parsing
    ui->treeView_organizer->setUpdatesEnabled(true);
    ui->treeView_objList->setUpdatesEnabled(true);
}

void MainTab_Chars::onManual_treeView_organizer_selectionChanged(const QModelIndex &selected, const QModelIndex& /* UNUSED deselected */ )
{
    if (m_organizer_model->rowCount() == 0)     // Empty list, can't proceed.
        return;

    QStandardItem *subsectionItem = m_organizer_model->itemFromIndex(selected);
    if (!m_subsectionMap.count(subsectionItem)) // If the selected item isn't in the map, it is a Category, not a Subsection.
        return;
    ScriptSubsection *subsectionInst = m_subsectionMap[subsectionItem];
    //if (subsectionItem == nullptr)   // Checking again (maybe useless, but...), because if the pointer isn't valid the program will crash.
    //    return;

    m_objList_model->removeRows(0,m_objList_model->rowCount());
    m_objMapQItemToScript.clear();
    m_objMapScriptToQItem.clear();

    ui->treeView_objList->setUpdatesEnabled(false);

    /* Populate the object list */
    for (ScriptObj* obj : subsectionInst->m_objects)
    {
        // Build the two QStandardItem for each ScriptObj in this Subsection
        QList<QStandardItem*> row;

        /* Build the description part */
        QStandardItem *description_item = new QStandardItem(obj->m_description.c_str());

        QString bodyStr;
        if (obj->m_baseDef)
            bodyStr = "Base Chardef";
        else
            bodyStr = "Child Chardef (Parent: " + QString(obj->m_ID.c_str()) + ")";
        QString colorStr;
        //if (isStringNumericHex(obj->m_color))
        //{
        //    int colorNum = ScriptUtils::strToSphereInt16(obj->m_color);
        //    colorStr = QString("0%1").arg(colorNum, 1, 16, QChar('0')) +
        //            " (Dec: " + QString("%1").arg(colorNum, 0, 10) + ")";
        //}
        //else
            colorStr = obj->m_color.c_str();

        description_item->setToolTip(
                    bodyStr                                                                     + "\n" +
                    "Color: "               + colorStr                                          + "\n" +
                    "Script File: "         + g_scriptFileList[obj->m_scriptFileIndex].c_str()  + "\n" +
                    "Script File Line: "    + QString::number(obj->m_scriptLine));

        if (obj->m_type == SCRIPTOBJ_TYPE_SPAWN)
            description_item->setForeground(QBrush(QColor("red")));

        row.append(description_item);

        /* Build the defname part */
        const std::string& def = obj->m_defname.empty() ? obj->m_ID : obj->m_defname;
        QStandardItem *defname_item = new QStandardItem(def.c_str());
        row.append(defname_item);

        /* Store the elements in the map and append the whole row to the view */
        m_objMapQItemToScript[description_item] = obj;    // For each row, only the Description item is "linked" to the obj via the map.
        m_objMapScriptToQItem[obj] = description_item;

        m_objList_model->appendRow(row);        
    }

    //m_objList_model->sort(0, Qt::AscendingOrder);   // Order alphabetically. -> not needed anymore, since the whole ScriptObjTree is now sorted after the parsing
    ui->treeView_objList->setUpdatesEnabled(true);
}

void MainTab_Chars::on_treeView_objList_doubleClicked(const QModelIndex &index)
{
    if (m_objList_model->rowCount() == 0)     // Empty list, can't proceed.
        return;

    //QStandardItem *objItem = m_objList_model->itemFromIndex(index);
    //if (!m_objMapQItemToScript.count(objItem)) // If the selected item isn't in the map there's something wrong.
    //    return;
    //ScriptObj *obj = m_objMapQItemToScript[objItem];

    QModelIndex IDIndex(m_objList_model->index(index.row(), 1, index.parent()));

    std::string strToSend = ".add " + IDIndex.data().toString().toStdString();
    auto ksResult = ks::KeystrokeSender::sendStringFastAsync(strToSend, true, g_sendKeystrokeAndFocusClient);
    if (ksResult != ks::KSError::Ok)
    {
        QMessageBox errorDlg(QMessageBox::Warning, "Warning", ks::getErrorStringStatic(ksResult), QMessageBox::NoButton, this);
        errorDlg.exec();
    }
}

void MainTab_Chars::onManual_treeView_objList_selectionChanged(const QModelIndex &selected, const QModelIndex& /* UNUSED deselected */ )
{
    if (g_UOAnim == nullptr)
        return;

    if (m_objList_model->rowCount() == 0)     // Empty list, can't proceed.
        return;

    QStandardItem *obj_item = m_objList_model->itemFromIndex(selected);
    if (!m_objMapQItemToScript.count(obj_item)) // If the selected item isn't in the map, it is a Category, not a Subsection.
        return;

    ScriptObj *script = m_objMapQItemToScript[obj_item];
    emit selectedScriptObjChanged(script);

    int id = script->m_display;
    ui->label_id->setText("ID: 0" + QString::number(id, 16) + " (" + QString::number(id, 10) + ")");

    int hue = ScriptUtils::strToSphereInt16(script->m_color);
    if (hue < 0)    // template or random expr (not supported yet) or strange string
        hue = 0;

    g_UOAnim->setHuesCachePointer(g_UOHues); // reset the right address (in case it has changed) to the hues to be used
    QImage* frameimg = g_UOAnim->drawAnimFrame(id, 0, 1, 0, hue);
    if (frameimg == nullptr)
        return;

    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(*frameimg));
    delete frameimg;
    if (ui->graphicsView->scene() != nullptr)
        delete ui->graphicsView->scene();
    QGraphicsScene* scene = new QGraphicsScene();
    ui->graphicsView->setScene(scene);
    scene->clear();
    scene->addItem(item);
}

void MainTab_Chars::on_pushButton_collapseAll_clicked()
{
    ui->treeView_organizer->collapseAll();
    ui->treeView_objList->collapseAll();
}

void MainTab_Chars::on_pushButton_summon_clicked()
{
    if (m_objList_model->rowCount() == 0)     // Empty list, can't proceed.
        return;
    QItemSelectionModel *selection = ui->treeView_objList->selectionModel();
    if (!selection->hasSelection())
        return;

    std::string strToSend = ".add " + selection->selectedRows(1)[0].data().toString().toStdString();
    auto ksResult = ks::KeystrokeSender::sendStringFastAsync(strToSend, true, g_sendKeystrokeAndFocusClient);
    if (ksResult != ks::KSError::Ok)
    {
        QMessageBox errorDlg(QMessageBox::Warning, "Warning", ks::getErrorStringStatic(ksResult), QMessageBox::NoButton, this);
        errorDlg.exec();
    }
}

void MainTab_Chars::on_pushButton_remove_clicked()
{
    std::string strToSend = ".remove";
    auto ksResult = ks::KeystrokeSender::sendStringFastAsync(strToSend, true, g_sendKeystrokeAndFocusClient);
    if (ksResult != ks::KSError::Ok)
    {
        QMessageBox errorDlg(QMessageBox::Warning, "Warning", ks::getErrorStringStatic(ksResult), QMessageBox::NoButton, this);
        errorDlg.exec();
    }
}

void MainTab_Chars::doSearch(bool backwards)
{
    ScriptObj* obj;
    if (!backwards)     // search forwards
        obj = m_scriptSearch->next();
    else                // search backwards
        obj = m_scriptSearch->previous();
    if (obj == nullptr) // not found
    {
        QMessageBox errorDlg(this);
        errorDlg.setText("Not found!");
        errorDlg.exec();
        return;
    }

    // look up the m_categoryMap map to see if we have loaded the QStandardItem corresponding to the ScriptCategory of the object
    ScriptCategory* category = obj->m_category;
    auto categoryIt = mapSearchByKey(m_categoryMap, category);          // it's an iterator
    if (categoryIt == m_categoryMap.end())      // not found? odd..
        return;
    //QStandardItem* categoryQ = categoryIt->first;

    // look up the m_subsectionMap map to see if we have loaded the QStandardItem corresponding to the ScriptSubsection of the object
    ScriptSubsection* subsection = obj->m_subsection;
    auto subsectionIt = mapSearchByKey(m_subsectionMap, subsection);    // it's an iterator
    if (subsectionIt == m_subsectionMap.end())  // not found? odd..
        return;
    QStandardItem* subsectionQ = subsectionIt->first;

    // now that i have the subsection's QStandardItem, get its QModelIndex
    QModelIndex emptyIdx;
    QModelIndex subsectionIdx = m_organizer_model->indexFromItem(subsectionQ);
    //QModelIndex categoryIdx = m_organizer_model->indexFromItem(categoryQ);

    // store the previous loaded subsection: if the current subsection is the same of the previous, there's no need to
    //  empty and load again the whole subsection list
    static QModelIndex prevSubsectionIdx;

    ui->treeView_organizer->scrollTo(subsectionIdx, QAbstractItemView::PositionAtCenter);
    //ui->treeView_organizer->setExpanded(categoryIdx, true);   // automatically expanded by the select method
    ui->treeView_organizer->selectionModel()->select(subsectionIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
    if (subsectionIdx != prevSubsectionIdx)
    {
        onManual_treeView_organizer_selectionChanged(subsectionIdx, emptyIdx);  // load the new subsection into treeView_objList
        prevSubsectionIdx = subsectionIdx;
    }

    QStandardItem* objQ = m_objMapScriptToQItem[obj];   // should check if found/not found?
    QModelIndex objIdx = m_objList_model->indexFromItem(objQ);

    onManual_treeView_objList_selectionChanged(objIdx, emptyIdx);
    ui->treeView_objList->setFocus();
    ui->treeView_objList->scrollTo(objIdx, QAbstractItemView::PositionAtCenter);
    ui->treeView_objList->selectionModel()->select(objIdx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Toggle);
}

void MainTab_Chars::on_pushButton_search_clicked()
{
    if (g_loadedScriptsProfile == -1)
        return;

    if (!m_subdlg_searchObj->exec())
        return;

    const std::vector<ScriptObjTree*> trees =
    {
        getScriptObjTree(SCRIPTOBJ_TYPE_CHAR),
        getScriptObjTree(SCRIPTOBJ_TYPE_SPAWN)
    };

    m_scriptSearch = std::make_unique<ScriptSearch>(trees, m_subdlg_searchObj->getSearchData());
    doSearch(false);
}

void MainTab_Chars::on_pushButton_search_back_clicked()
{
    if (g_loadedScriptsProfile == -1)   // no profile loaded
        return;
    if (m_scriptSearch == nullptr)      // i haven't set the search parameters
        return;

    doSearch(true);     // search backwards
}

void MainTab_Chars::on_pushButton_search_next_clicked()
{
    if (g_loadedScriptsProfile == -1)   // no profile loaded
        return;
    if (m_scriptSearch == nullptr)      // i haven't set the search parameters
        return;

    doSearch(false);    // search forwards
}

void MainTab_Chars::on_pushButton_spawner_clicked()
{
    SubDlg_Spawn* spawner = new SubDlg_Spawn(this);
    connect(this, SIGNAL(selectedScriptObjChanged(ScriptObj*)), spawner, SLOT(onCust_selectedObj_changed(ScriptObj*)));
    spawner->show();

    // get the selected object and update the spawner dialog with the current object
    QItemSelectionModel *selection = ui->treeView_objList->selectionModel();
    if (!selection->hasSelection())
        return;
    QStandardItem* qitem = m_objList_model->itemFromIndex( selection->selectedRows(0)[0] );
    ScriptObj *script = m_objMapQItemToScript[qitem];
    emit selectedScriptObjChanged(script);
}
