#include "RoomDialog.h"

QString Roomname1;

RoomDialog::RoomDialog(QWidget  * parent1) :
 QDialog(parent1)
{
 setUpGUI();
 setWindowTitle( tr("Create Room") );
 setModal( true );
}

void RoomDialog::setUpGUI(){
 // set up the layout
 QGridLayout * formGridLayout = new QGridLayout( this );

// initialize the username combo box so that it is editable
 editRoomname = new QLineEdit( this );
 //comboRoomname->setEditable( true );
 // initialize the password field so that it does not echo
 // characters

// initialize the labels
 labelRoomname = new QLabel( this );
 labelRoomname->setText( tr( "Room Name" ) );
 labelRoomname->setBuddy( editRoomname );

// initialize buttons
 buttons = new QDialogButtonBox( this );
 buttons->addButton( QDialogButtonBox::Ok );
 buttons->addButton( QDialogButtonBox::Cancel );
 buttons->button( QDialogButtonBox::Ok )->setText( tr("OK") );
 buttons->button( QDialogButtonBox::Cancel )->setText( tr("Cancel") );

 // connects slots
 connect( buttons->button( QDialogButtonBox::Cancel ),
 SIGNAL (clicked()),
 this,
 SLOT (close())
 );

connect( buttons->button( QDialogButtonBox::Ok ),
 SIGNAL (clicked()),
 this,
 SLOT (slotacceptLogin()) );

// place components into the dialog
 formGridLayout->addWidget( labelRoomname, 0, 0 );
 formGridLayout->addWidget( editRoomname, 0, 1 );
 formGridLayout->addWidget( buttons, 2, 0, 1, 2 );

setLayout( formGridLayout );

}



void RoomDialog::slotacceptLogin(){
 Roomname1 = editRoomname->text();


// close this dialog
 close();
}
QString RoomDialog::getRoomname() {
    return Roomname1;
}
