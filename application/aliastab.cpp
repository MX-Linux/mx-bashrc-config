﻿#include "aliastab.h"

#include "searcher.h"
#include <QTextStream>
#include <QCheckBox>
#include <algorithm>
#include <QString>

AliasTab::AliasTab()
    : Tab("Aliases")
{
    SCOPE_TRACKER;
    ui = new Ui::AliasTab;
    setWidget(new QWidget());
    ui->setupUi(widget());
    ui->tableWidget_Aliases->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_Aliases->setColumnCount(2);
    ui->tableWidget_Aliases->setHorizontalHeaderLabels(QStringList() << "Alias" << "Command");
    ui->tableWidget_Aliases->setSelectionBehavior(QTableWidget::SelectRows);

    connect(ui->pushButton_AliasAdd, &QPushButton::clicked, [=](){
        ui->tableWidget_Aliases->setRowCount(ui->tableWidget_Aliases->rowCount()+1);
        ui->tableWidget_Aliases->setItem(ui->tableWidget_Aliases->rowCount()-1, 0, new AliasTabTableWidgetItem("", false));
        ui->tableWidget_Aliases->setItem(ui->tableWidget_Aliases->rowCount()-1, 1, new AliasTabTableWidgetItem(""));
    });
    connect(ui->pushButton_AliasRemove, &QPushButton::clicked, [=](){
        for(int i = ui->tableWidget_Aliases->rowCount()-1; i > -1; i--)
        {
            if(ui->tableWidget_Aliases->item(i,0) != nullptr &&  ui->tableWidget_Aliases->item(i,0)->isSelected())
            {
                Alias adata;
                adata.setAlias(ui->tableWidget_Aliases->item(i, 0)->text());
                adata.setCommand(ui->tableWidget_Aliases->item(i, 1)->text());
                m_deletedAliases << adata;
                ui->tableWidget_Aliases->removeRow(i);
            }
        }
    });
}

AliasTab::~AliasTab()
{
    SCOPE_TRACKER;
    delete ui;
    m_aliasWithCheckboxes.clear();
}

void AliasTab::setup(const BashrcSource data)
{
    SCOPE_TRACKER;
    bool doSuggestions = true;
    QFile suggestionAliases(SUGGEST_ALIASES);
    QList<Alias> suggestionAliasesList;
    if(!suggestionAliases.open(QFile::Text | QFile::ReadOnly))
    {
        DEBUG << suggestionAliases.fileName() + " isn't readable, isn't text, or doesn't exist";
        DEBUG << "Not loading suggestions";
        doSuggestions = false;
    }
    if(doSuggestions)
    {
        QTextStream stream(&suggestionAliases);
        QString content = stream.readAll();
        AliasStream aliasStream(&content);
        suggestionAliasesList = aliasStream.get();
        for(auto alias : suggestionAliasesList)
        {
            m_aliasWithCheckboxes[new QCheckBox(alias.alias() + " - " + alias.command())] = alias;
        }
        for(auto key : m_aliasWithCheckboxes.keys())
        {
            ui->verticalLayout_SuggestionAliases->addWidget(key);
            connect(key, &QCheckBox::clicked, [=](bool checked){
                if(!checked)
                    m_deletedAliases.append(m_aliasWithCheckboxes[key]);
            });
        }
    }
    AliasStream bashrcAliasStream(new QString(data.bashrc), true);
    AliasStream programAliasStream(new QString(data.program));
    AliasStream bashrcAliasesAliasStream(new QString(data.bashrcAliases));

    QList<Alias> aliases;
    aliases.append(bashrcAliasStream.get());
    aliases.append(bashrcAliasesAliasStream.get());
    aliases.append(programAliasStream.get());

    for(auto alias : aliases)
    {
        bool addToTable = true;
        for(auto key : m_aliasWithCheckboxes.keys())
        {
            if(m_aliasWithCheckboxes[key] == alias)
            {
                key->setChecked(true);
                addToTable = false;
                break;
            }
        }
        if(addToTable)
        {
            ui->tableWidget_Aliases->setRowCount(ui->tableWidget_Aliases->rowCount()+1);
            ui->tableWidget_Aliases->setItem(ui->tableWidget_Aliases->rowCount()-1, 0, new AliasTabTableWidgetItem(alias.alias(), alias.inBashrc()));
            ui->tableWidget_Aliases->setItem(ui->tableWidget_Aliases->rowCount()-1, 1, new AliasTabTableWidgetItem(alias.command()));
        }
    }
}

BashrcSource AliasTab::exec(const BashrcSource data)
{
    SCOPE_TRACKER;

    BashrcSource rtn = data;

    /* HAVING TROUBLE but good code
    bool sendToBashAliases = false;

    //Possible issues: if the content of the macro USER_BASHRC_ALIASES does change it will break this regex
    if(QFile(USER_BASHRC_ALIASES).exists() && rtn.bashrc.contains(QRegularExpression("^[^#](\\.|source)[\s]+(\\$HOME|~|/home/sd)/(.bash_aliases")))
        sendToBashAliases = true;
    */

    AliasStream bashrcAliasStream(&rtn.bashrc);
    AliasStream programAliasStream(&rtn.program);
    AliasStream bashrcAliasesAliasStream(&rtn.bashrcAliases);

    for(Alias alias : m_deletedAliases)
    {
        //remove from all files and locations
        bashrcAliasesAliasStream.remove(alias);
        bashrcAliasStream.remove(alias);
        programAliasStream.remove(alias);
    }

    for(int row = 0; row < ui->tableWidget_Aliases->rowCount(); row++)
    {
        // if empty field in either field skip over this alias
        if(ui->tableWidget_Aliases->item(row, 0)->text().isEmpty() ||
                ui->tableWidget_Aliases->item(row, 1)->text().isEmpty())
            continue;
        Alias alias(ui->tableWidget_Aliases->item(row, 0)->text(),
                    ui->tableWidget_Aliases->item(row, 1)->text());
        alias.setInBashrc(static_cast<AliasTabTableWidgetItem*>(ui->tableWidget_Aliases->item(row, 0))->info().toBool());
        if(alias.inBashrc())
        {
            DEBUG << "Alias: " << alias.alias() << " : " << alias.command() << " has been detected as a bashrc alias";
            bashrcAliasStream << alias;
        }
        else
        {
            bashrcAliasesAliasStream << alias;
        }
    }

    for(auto key : m_aliasWithCheckboxes.keys())
    {
        if(key->isChecked())
        {
            bashrcAliasesAliasStream << m_aliasWithCheckboxes[key];
        }
    }

    return rtn;
}

AliasTabTableWidgetItem::AliasTabTableWidgetItem(QString text, QVariant info)
{
    SCOPE_TRACKER;
    setText(text);
    setInfo(info);
}

AliasTabTableWidgetItem::~AliasTabTableWidgetItem()
{
    SCOPE_TRACKER;
     //None
}

AliasTabTableWidgetItem &AliasTabTableWidgetItem::setInfo(QVariant info)
{
    SCOPE_TRACKER;
    m_info = info;
    return *this;
}

//temp fix QVarient crashes program
QVariant& AliasTabTableWidgetItem::info()
{
    SCOPE_TRACKER;
    return m_info;
}
