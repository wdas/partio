#!/usr/bin/env python
"""
A fast spreadsheet view of a partio file (e.g. bgeo)
Usage:
    partinspect [FLAGS] filename

Supported FLAGS:
    -a/--attrs: Only show attribute information, not particle data
    -h/--help : Print this help message
"""

import os, sys, re
from Qt import QtCore, QtWidgets # pylint:disable=E0611
from Qt.QtGui import QKeySequence # pylint:disable=E0611,E0401
from Qt.QtCore import Qt # pylint:disable=E0611,E0401
import partio

#--------------------------------------------------------------------------------------
_attrTypeNames = ('None', 'Vector', 'Float', 'Integer', 'Indexed String')
class PartioTreeModel(QtCore.QAbstractItemModel):
    """A model that provides a Qt access to PARTIO data (read only)"""
    def __init__(self, parent, parts, attrsOnly=False):
        QtCore.QAbstractItemModel.__init__(self, parent)
        self.attrsOnly = attrsOnly
        self.parts=parts
        self.attribs=[]

        numAttr = self.parts.numAttributes()
        nameToIndex = {self.parts.attributeInfo(anum).name:anum for anum in range(numAttr)}
        sortedNames = sorted(nameToIndex.keys())
        names = []
        specials = ('id', 'h_pil_id')
        for special in specials:
            if special in sortedNames:
                names.append(special)
                sortedNames.remove(special)
        names += sortedNames
        for name in names:
            i = nameToIndex[name]
            attrib = self.parts.attributeInfo(i)
            for j in range(attrib.count):
                self.attribs.append( (attrib, j) )

        if attrsOnly:
            self.numRows = len(self.attribs)
            self.numColumns = 4
        else:
            self.numRows = self.parts.numParticles()
            self.numColumns = len(self.attribs) + 1

    def columnCount(self, parent): # pylint:disable=W0613
        """ Return number of columns """
        return self.numColumns

    def rowCount(self,parent):
        """ Return number of rows in the spreadsheet """
        if parent.isValid():
            return 0
        return self.numRows

    def data(self, index, role): # pylint:disable=W0613
        """ Returns data value """

        if not index.isValid():
            return None

        row = index.row()
        if row < 0 or row >= self.numRows:
            return None
        if role!=QtCore.Qt.EditRole and role !=QtCore.Qt.DisplayRole:
            return None

        col = index.column()

        if self.attrsOnly:
            if row >= 0:
                attrib, _ = self.attribs[row]
                if col == 1:
                    return attrib.name
                if col == 2:
                    return _attrTypeNames[attrib.type]
                if col == 3:
                    return attrib.count
        else:
            if col > 0:
                attrib, datacol = self.attribs[col-1]
                data=self.parts.get(attrib,row)[datacol]
                return data

        return str(row)

    def headerData(self, section, orientation, role): # pylint:disable=W0613
        """ Return the header label """
        if role != QtCore.Qt.DisplayRole:
            return None

        if self.attrsOnly:
            try:
                return ['Index', 'Attribute', 'Type', 'Count'][section]
            except IndexError:
                return None
        else:
            if section>0:
                return "%s[%d]"%(self.attribs[section-1][0].name,self.attribs[section-1][1])
            else:
                return "Index"

    def index(self, row, column, parent): # pylint:disable=W0613
        """ Return an index for the given row/col """
        return self.createIndex(row, column, 0)

    def parent(self, index): # pylint:disable=W0613,R0201
        """ Return the parent model index """
        return QtCore.QModelIndex()

#--------------------------------------------------------------------------------------
class FilterModel(QtCore.QSortFilterProxyModel):
    """ A filter proxy model for the particle data """

    def __init__(self, lineEditWidget, attrsOnly, parent):
        QtCore.QSortFilterProxyModel.__init__(self, parent)
        self.lineEdit = lineEditWidget
        self.attrsOnly = attrsOnly

    def filterAcceptsColumn(self, source_column, source_parent): # pylint:disable=W0613
        """ Determines if column matches filter """

        if self.attrsOnly:
            return True

        attrName = self.sourceModel().headerData(source_column, Qt.Vertical, QtCore.Qt.DisplayRole)
        if attrName == 'Index':
            return True
        filterText = self.lineEdit.text()
        if not filterText:
            return True
        parts = re.split(r'\W+', filterText)
        return any([part in attrName for part in parts if part])

    def filterAcceptsRow(self, source_row, source_parent): # pylint:disable=W0613
        """ Determines if row (attribute) matches filter """

        if not self.attrsOnly:
            return True

        index = self.sourceModel().createIndex(source_row, 1)
        attrName = self.sourceModel().data(index, QtCore.Qt.DisplayRole)
        filterText = self.lineEdit.text()
        if not filterText:
            return True
        parts = re.split(r'\W+', filterText)
        return any([part in attrName for part in parts if part])

    def filter(self, text): # pylint:disable=W0613
        """ Filters the table based on the glob expression """
        self.invalidateFilter()


#--------------------------------------------------------------------------------------
class Spreadsheet(QtWidgets.QDialog):
    """ Makes a spreadsheet data view using partio of the particle file specified.
        Normally, displays attribute per column, particle per row.
        If attrsOnly is True, display attribute per row and attribute info per column.
    """

    def __init__(self, filename, attrsOnly=False, parent=None):
        QtWidgets.QDialog.__init__(self, parent)
        self.treeView = None
        self.hasParticles = False
        self.lineEdit = None
        self.attrsOnly = attrsOnly
        self.filename = None

        vbox = QtWidgets.QVBoxLayout()
        self.setLayout(vbox)
        hbox = QtWidgets.QHBoxLayout()
        vbox.addLayout(hbox)
        hbox.addWidget(QtWidgets.QLabel("Attribute Filter:"))
        self.lineEdit = QtWidgets.QLineEdit()
        hbox.addWidget(self.lineEdit)

        # Configure ctrl-w to close window
        QtWidgets.QShortcut( QKeySequence(Qt.CTRL + Qt.Key_W), self, self.accept )

        self.setFile(filename)

    def setFile(self, filename):
        """ Redraws the spreadsheet with a new particle file """

        if filename == self.filename:
            return
        self.filename = filename

        # reusing empty list causes column resizing problem so delete it
        if self.treeView and not self.hasParticles:
            self.layout().removeWidget(self.treeView)
            del self.treeView
            self.treeView = None
        if not self.treeView:
            self.treeView = QtWidgets.QTreeView()
            self.treeView.setRootIsDecorated(False)
            self.treeView.setUniformRowHeights(True)
            self.layout().addWidget(self.treeView)

        p = partio.read(filename)
        if not p:
            p = partio.create()
            self.hasParticles = False
        else:
            self.hasParticles = True

        model = PartioTreeModel(self.treeView, p, self.attrsOnly)
        proxyModel = FilterModel(self.lineEdit, self.attrsOnly, self)
        self.lineEdit.textChanged.connect(proxyModel.filter)
        proxyModel.setSourceModel(model)
        self.treeView.setModel(proxyModel)
        for i in range(model.columnCount(self.treeView)):
            self.treeView.resizeColumnToContents(i)
        self.setWindowTitle(filename)
        self.show()

_particleSpreadsheet = None
_attrSpreadsheet = None

def spreadsheet(filename=None, attrsOnly=False, parent=None):
    """ Return a singleton copy of a Spreadsheet object """
    if attrsOnly:
        global _attrSpreadsheet
        if not filename and _attrSpreadsheet:
            return _attrSpreadsheet
        if not _attrSpreadsheet:
            _attrSpreadsheet = Spreadsheet(filename, attrsOnly, parent)
        else:
            _attrSpreadsheet.setFile(filename)
        return _attrSpreadsheet

    global _particleSpreadsheet
    if not filename and _particleSpreadsheet:
        return _particleSpreadsheet
    if not _particleSpreadsheet:
        _particleSpreadsheet = Spreadsheet(filename, attrsOnly, parent)
    else:
        _particleSpreadsheet.setFile(filename)

def isVisible(attrsOnly=False):
    """ See if a spreadsheet was built and is visible """
    if attrsOnly:
        return _attrSpreadsheet and _attrSpreadsheet.isVisible()
    return _particleSpreadsheet and _particleSpreadsheet.isVisible()

#--------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------
def main():
    """ main """

    # Process command-line flags
    attrsOnly = False
    filename = None
    newArgs = [sys.argv[0]]
    for arg in sys.argv[1:]:
        if arg in ('-h', '--help'):
            print(__doc__)
            sys.exit(0)
        if arg in ('-a', '--attrs'):
            attrsOnly = True
        elif not arg.startswith('-'):
            filename = arg
        else:
            newArgs.append(arg)

    if not filename:
        print(__doc__)
        sys.stderr.write("Missing filename\n")
        sys.exit(1)

    if not os.path.exists(filename):
        print(__doc__)
        sys.stderr.write("Invalid file: {}\n".format(filename))
        sys.exit(1)

    # Create the dialog
    app = QtWidgets.QApplication(newArgs)
    dialog = Spreadsheet(filename, attrsOnly)

    # Configure ctrl-q to quit
    QtWidgets.QShortcut( QKeySequence(Qt.CTRL + Qt.Key_Q), dialog, dialog.close )
    app.exec_()

if __name__=="__main__":
    main()
