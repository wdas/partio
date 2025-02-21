#!/usr/bin/env python
# pylint:disable=C0302

"""
An interactive spreadsheet for viewing, editing, and saving
partio (bgeo) files.

Usage:
  % partedit [FLAGS] [bgeoFile]

Supported FLAGS:
  -h/--help: Print this help message

"""

# TODO:
# Support for fixed attribute delete and rename
# Support for indexed strings
# Tighten up particle table whitespace usage (smaller font? popup matrix?)
# Performance - delay widget construction

# NEXT UP:
# - delete fixed attribute
# - rename fixed attribute

import math
import os
import sys

# pylint:disable=E0611,E0401
from Qt.QtGui import QKeySequence, QIcon, QIntValidator, QDoubleValidator, QFontMetrics
from Qt.QtWidgets import QShortcut, QApplication, QMainWindow, \
    QPushButton, QTableWidget, QLabel, QWidget, QVBoxLayout, QHeaderView,\
    QHBoxLayout, QLineEdit, QFileDialog, QFrame, QDialog, QFormLayout, \
    QComboBox, QCheckBox, QTableWidgetItem, QSplitter, QSizePolicy
from Qt.QtCore import Qt, QSize, QObject, Signal

import partio


#------------------------------------------------------------------------------_
_attrTypes = [partio.NONE, partio.VECTOR, partio.FLOAT, partio.INT, partio.INDEXEDSTR]

#------------------------------------------------------------------------------
def copy(srcData):
    """ Creates a copy of the given partio data set """

    dstData = partio.create()
    srcAttrs = []
    dstAttrs = []
    for anum in range(srcData.numAttributes()):
        attr = srcData.attributeInfo(anum)
        srcAttrs.append(attr)
        dstAttrs.append(dstData.addAttribute(attr.name, attr.type, attr.count))
    dstData.addParticles(srcData.numParticles())
    for pnum in range(srcData.numParticles()):
        for anum, srcAttr in enumerate(srcAttrs):
            dstData.set(dstAttrs[anum], pnum, srcData.get(srcAttr, pnum))
    return dstData

#--------------------------------------------------------------------------
def getAttrs(numAttributesFunc, attributeInfoFunc, sort=False):
    """ Return list of tuples of (attributeNum, attribute) """
    attrs = []
    numAttr = numAttributesFunc()

    nameToIndex = {attributeInfoFunc(anum).name:anum for anum in range(numAttr)}
    names = list(nameToIndex.keys())
    if sort:
        names.sort()

    id_offset = 0
    for name in names:
        anum = nameToIndex[name]
        attr = attributeInfoFunc(anum)
        if sort and attr.name == 'id':
            attrs.insert(0, (anum, attr))
            id_offset += 1
        elif sort and 'id' in attr.name:
            attrs.insert(id_offset, (anum, attr))
            id_offset += 1
        else:
            attrs.append((anum, attr))

    return attrs

#--------------------------------------------------------------------------
def copyParticles(src, dst):
    """ Copy particles from src to dst. """

    # Identify the attributes that are in both src and dst
    srcAttrs = [src.attributeInfo(i) for i in range(src.numAttributes())]
    dstAttrs = [dst.attributeInfo(i) for i in range(dst.numAttributes())]
    srcAttrs = {attr.name:attr for attr in srcAttrs}
    dstAttrs = {attr.name:attr for attr in dstAttrs}
    attrs = {'src':[], 'dst':[]}
    for name, srcAttr in srcAttrs.iteritems():
        if name in dstAttrs:
            attrs['src'].append(srcAttr)
            attrs['dst'].append(dstAttrs[name])

    numParticles = src.numParticles()
    dst.addParticles(numParticles)
    for pnum in range(numParticles):
        for anum in range(len(attrs)):
            dst.set(attrs['dst'][anum], pnum, src.get(attrs['src'][anum], pnum))

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
class ParticleData(QObject):
    """ UI Controller class for partio data """

    particleAdded = Signal(int)
    attributeAdded = Signal(str)
    fixedAttributeAdded = Signal(str)
    dataReset = Signal()
    dirtied = Signal(bool)

    def __init__(self):
        QObject.__init__(self)
        self.setData(partio.create())
        self.filename = None
        self.dirty = False

    #--------------------------------------------------------------------------
    def setDirty(self, dirty):
        """ Stores the dirty state of the data """

        if dirty != self.dirty:
            self.dirty = dirty
            self.dirtied.emit(dirty)

    #--------------------------------------------------------------------------
    def setData(self, data):
        """ Sets the data, linking class methods to partio methods and
            notifying all observers that the data set has changed.
        """
        self.data = data
        self.originalData = copy(data)
        self.facade()
        self.dataReset.emit()

    #--------------------------------------------------------------------------
    def facade(self):
        """ Facades methods through to data """

        self.get = self.data.get
        self.getFixed = self.data.getFixed
        self.numAttributes = self.data.numAttributes
        self.numFixedAttributes = self.data.numFixedAttributes
        self.numParticles = self.data.numParticles
        self.attributeInfo = self.data.attributeInfo
        self.fixedAttributeInfo = self.data.fixedAttributeInfo
        self.indexedStrs = self.data.indexedStrs
        self.fixedIndexedStrs = self.data.fixedIndexedStrs

    #--------------------------------------------------------------------------
    def set(self, *args):
        """ Sets a value on the partio data, marking dirty. """

        self.setDirty(True)
        self.data.set(*args)

    #--------------------------------------------------------------------------
    def setFixed(self, *args):
        """ Sets a fixed attribute value on the partio data, marking dirty. """

        self.setDirty(True)
        self.data.setFixed(*args)

    #--------------------------------------------------------------------------
    def read(self, filename):
        """ Opens a file from disk and populates the UI """

        if not os.path.exists(filename):
            sys.stderr.write('Invalid filename: {}\n'.format(filename))
            return

        data = partio.read(filename)
        if not data:
            sys.stderr.write('Invalid particle file: {}\n'.format(filename))
            data = partio.create()

        self.filename = filename
        self.setData(data)
        self.setDirty(False)

    #--------------------------------------------------------------------------
    def write(self, filename, delta):
        """ Write data to file. If delta is False, saves a full copy
            of the data, rebaselining. If delta is True, saves only
            the particles (todo: and attributes) that have changed,
            but maintains the original baseline
        """

        if not self.data:
            return

        # If we're saving a delta, create a new particle set with just
        # the differences from the original.
        if delta:
            data = self.createDelta()
        else:
            data = self.data

        partio.write(filename, data)

        # If we saved a full copy, rebaseline
        if not delta:
            self.filename = filename
            self.originalData = copy(data)
            self.setDirty(False)

    #--------------------------------------------------------------------------
    def createDelta(self):
        """ Creates a delta particle set between the current and original
            data set. This is the brute-force method, simply comparing the
            current data set against the original, but it's easier than
            tracking individual changes.
        """

        def hashParticles(data):
            """ Given a partio data set, create a dictionary of hashes
                to indices
            """
            items = {}
            numAttrs = data.numAttributes()
            for pnum in range(data.numParticles()):
                item = []
                for anum in range(numAttrs):
                    attr = data.attributeInfo(anum)
                    item.append(data.get(attr, pnum))
                items[hash(str(item))] = pnum
            return items

        # TODO: Handle new attributes as deltas
        # For now, any new attributes will write all of the particles

        # Hash up the new data into an index table
        newParticles = hashParticles(self.data)
        oldParticles = hashParticles(self.originalData)

        # If nothing changed, easy out
        data = partio.create()
        if newParticles == oldParticles:
            return data

        # Identify which particles changed
        oldHashes = set(oldParticles.keys())
        newHashes = set(newParticles.keys())
        modifiedHashes = newHashes - oldHashes

        # Create the new particle set
        numAttrs = self.data.numAttributes()
        newAttrs = []
        oldAttrs = []
        for anum in range(numAttrs):
            attr = self.data.attributeInfo(anum)
            oldAttrs.append(attr)
            newAttr = data.addAttribute(attr.name, attr.type, attr.count)
            newAttrs.append(newAttr)

        data.addParticles(len(modifiedHashes))
        for newIndex, modifiedHash in enumerate(modifiedHashes):
            oldIndex = newParticles[modifiedHash]
            for anum, oldAttr in enumerate(oldAttrs):
                value = self.data.get(oldAttr, oldIndex)
                data.set(newAttrs[anum], newIndex, value)

        return data

    #--------------------------------------------------------------------------
    def addParticle(self):
        """ Adds a new particle, emitting its new index.
            The new particle's values are copied from the last particle.
            If the particle set has the 'id' attribute, the new
            particle id is set to max(ids)+1.
        """

        if not self.data:
            return

        numParticles = self.numParticles()
        index = self.data.addParticle()
        numAttr = self.numAttributes()

        idAttr = self.attributeInfo('id')
        if idAttr:
            newId = max(self.data.get(idAttr, pnum)[0] for pnum in range(numParticles)) + 1

        for anum in range(numAttr):
            attr = self.attributeInfo(anum)
            if idAttr and attr.name == 'id':
                value = (newId,)
            else:
                value = self.get(attr, numParticles-1)
            self.set(attr, numParticles, value)

        self.particleAdded.emit(index)
        self.setDirty(True)

    #--------------------------------------------------------------------------
    def removeParticles(self, indices):
        """ Removes the particles at the given indices.
            partio doesn't support removing data, so we have
            to construct all new data sans the given particle
        """

        for anum in range(self.data.numAttributes()):
            attr = self.data.attributeInfo(anum)
        attributes = [self.data.attributeInfo(anum) for anum in range(self.data.numAttributes())]

        want = [pnum for pnum in range(self.data.numParticles()) if pnum not in indices ]
        newData = partio.clone(self.data, False)
        for attr in attributes:
            newData.addAttribute(attr.name, attr.type, attr.count)
        newData.addParticles(len(want))
        for i, idx in enumerate(want):
            for attr in attributes:
                newData.set(attr, i, self.data.get(attr, idx))

        self.setData(newData)
        self.setDirty(True)

    #--------------------------------------------------------------------------
    def addAttribute(self, name, attrType, count, fixed, defaultValue):
        """ Adds a new attribute for the particles, returning a
            handle to the new attribute.
        """

        if not isinstance(defaultValue, tuple):
            defaultValue = (defaultValue,)

        if fixed:
            attr = self.data.addFixedAttribute(name, attrType, count)
            self.data.setFixed(attr, defaultValue)
            self.fixedAttributeAdded.emit(attr.name)
        else:
            attr = self.data.addAttribute(name, attrType, count)
            for pnum in range(self.numParticles()):
                self.data.set(attr, pnum, defaultValue)
            self.attributeAdded.emit(attr.name)

        self.setDirty(True)

    #--------------------------------------------------------------------------
    def removeAttributes(self, names):
        """ Removes the attributes with the given names.
            partio doesn't support removing data, so we have
            to construct all new data sans the given attribute(s).
        """

        newData = partio.create()
        for anum in range(self.numAttributes()):
            attr = self.attributeInfo(anum)
            if attr.name not in names:
                newData.addAttribute(attr.name, attr.type, attr.count)

        # Copy particle data with new attributes
        copyParticles(src=self.data, dst=newData)

        # Copy fixed attributes
        for anum in range(self.data.numFixedAttributes()):
            oldAttr = self.data.fixedAttributeInfo(anum)
            newAttr = newData.addFixedAttribute(oldAttr.name, oldAttr.type, oldAttr.count)
            newData.setFixed(newAttr, self.data.getFixed(oldAttr))

        self.setData(newData)
        self.setDirty(True)


    #--------------------------------------------------------------------------
    def removeFixedAttributes(self, names):
        """ Removes the fixed attributes with the given names.
            partio doesn't support removing data, so we have
            to construct all new data sans the given attribute(s).
        """

        newData = partio.create()

        # Copy the regular (non-fixed) attributes and particles
        for anum in range(self.data.numAttributes()):
            attr = self.attributeInfo(anum)
            newData.addAttribute(attr.name, attr.type, attr.count)
        copyParticles(src=self.data, dst=newData)

        # Create new fixed attributes
        for anum in range(self.data.numFixedAttributes()):
            srcAttr = self.fixedAttributeInfo(anum)
            if srcAttr.name not in names:
                dstAttr = newData.addFixedAttribute(srcAttr.name, srcAttr.type, srcAttr.count)
                newData.setFixed(dstAttr, self.data.getFixed(srcAttr))

        self.setData(newData)
        self.setDirty(True)

#------------------------------------------------------------------------------
class NumericalEdit(QLineEdit): # pylint:disable=R0903
    """ A LineEdit that auto installs a validator for numerical types """

    def __init__(self, value, parent=None):
        QLineEdit.__init__(self, str(value), parent)
        self.setAlignment(Qt.AlignRight)
        self.setMinimumWidth(50)
        self.setCursorPosition(0)
        if isinstance(value, int):
            self.setValidator(QIntValidator())
        elif isinstance(value, float):
            self.setValidator(QDoubleValidator())

    def sizeHint(self):
        hint = super().sizeHint()
        fm = QFontMetrics(self.font())
        stringWidth = fm.width(self.text() + '00')  # add a couple characters for padding
        hint.setWidth(stringWidth)
        return hint


#------------------------------------------------------------------------------
class AttrWidget(QFrame): # pylint:disable=R0903
    """ The primary widget for table entries representing a particle attribute """

    widgetNumber = 0

    def __init__(self, value, data, attr, particleNum, numColumns, parent=None):
        QWidget.__init__(self, parent)
        self.value = value
        self.data = data
        self.attr = attr
        self.particleNum = particleNum
        self.setFrameShape(QFrame.NoFrame)

        self.name = 'AttrWidget{}'.format(AttrWidget.widgetNumber)
        self.setObjectName(self.name)
        AttrWidget.widgetNumber += 1
        self.withBorderStyle = '#%s {border: 1px solid dodgerblue;}' % self.name
        self.noBorderStyle = '#%s {border: 0px;}' % self.name
        self.setStyleSheet(self.noBorderStyle)

        layout = QVBoxLayout()
        layout.setContentsMargins(0,0,0,0)
        layout.setSpacing(0)
        self.setLayout(layout)

        idx = 0
        self.items = []
        self.textValues = []
        if numColumns:
            numRows = int(math.ceil(len(value) / float(numColumns)))
        else:
            numRows = 0
        for _ in range(numRows):
            row = QHBoxLayout()
            layout.addLayout(row)
            for _ in range(numColumns):
                item = NumericalEdit(value[idx])
                self.textValues.append(str(value[idx]))
                item.editingFinished.connect(self.applyEdit)
                row.addWidget(item, Qt.AlignHCenter|Qt.AlignTop)
                self.items.append(item)
                idx += 1
                if idx == len(self.value):
                    break

    #--------------------------------------------------------------------------
    def applyEdit(self):
        """ Callback when editing finished on a cell. Sets data value. """

        newValue = []
        changed = False
        for i, item in enumerate(self.items):
            text = item.text()
            if text != self.textValues[i]:
                changed = True
            if isinstance(self.value[0], int):
                newValue.append(int(text))
            else:
                newValue.append(float(text))
            item.clearFocus()
        if changed:
            self.value = tuple(newValue)
            if self.particleNum >= 0:
                self.data.set(self.attr, self.particleNum, self.value)
            else:
                self.data.setFixed(self.attr, self.value)
            self.drawBorder(True)

    #--------------------------------------------------------------------------
    def drawBorder(self, border):
        """ Sets or clears the border around the frame """

        if border:
            self.setStyleSheet(self.withBorderStyle)
        else:
            self.setStyleSheet(self.noBorderStyle)

#------------------------------------------------------------------------------
def getWidget(value, data, attr, particleNum=-1):
    """ Returns the correct type of QWidget based off of the item type.
        A particleNum<0 means a fixed attribute.
    """

    if isinstance(value, tuple):
        size = len(value)
        if size == 16:
            result = AttrWidget(value, data, attr, particleNum, 4)
        elif size == 9:
            result = AttrWidget(value, data, attr, particleNum, 3)
        else:
            result = AttrWidget(value, data, attr, particleNum, size)
    else:
        result = QLabel(str(value))
    return result


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
class ParticleTableWidget(QTableWidget): # pylint:disable=R0903
    """ A QTableWidget interfacing with ParticleData"""

    def __init__(self, data, parent=None):
        QTableWidget.__init__(self, parent)
        self.data = data

        # Connect data signals to my slots
        self.data.particleAdded.connect(self.particleAddedSlot)
        self.data.attributeAdded.connect(self.attributeAddedSlot)
        self.data.dataReset.connect(self.dataResetSlot)
        self.data.dirtied.connect(self.dataDirtiedSlot)

        style = 'QTableWidget::item { border: 1px solid gray; }'
        self.setStyleSheet(style)
        self.ignoreSignals = False
        self.populate()

    #--------------------------------------------------------------------------
    def populate(self):
        """ Populate the table with the data """

        self.clear()

        numAttr = self.data.numAttributes()
        numParticles = self.data.numParticles()

        self.attrs = getAttrs(self.data.numAttributes, self.data.attributeInfo, True)
        self.setColumnCount(numAttr)
        self.setRowCount(numParticles)
        self.horizontalHeader().setSectionResizeMode(QHeaderView.Interactive)
        for col, (_, attr) in enumerate(self.attrs):
            item = QTableWidgetItem(attr.name)
            tooltip = '<p><tt>&nbsp;Name: {}<br>&nbsp;Type: {}<br>Count: {}</tt></p>'.\
                      format(attr.name, partio.TypeName(attr.type), attr.count)
            item.setToolTip(tooltip)
            self.setHorizontalHeaderItem(col, item)
        self.horizontalHeader().setStretchLastSection(False)
        self.setVerticalHeaderLabels([str(pnum) for pnum in range(numParticles)])
        self.setTabKeyNavigation(True)
        self.horizontalHeader().setSectionsMovable(False)

        # Populate it with the particle data
        self.widgets = []
        for pnum in range(numParticles):
            self.populateParticle(pnum)

        self.horizontalHeader().resizeSections(QHeaderView.ResizeToContents)
        self.verticalHeader().resizeSections(QHeaderView.ResizeToContents)

    #--------------------------------------------------------------------------
    def populateParticle(self, pnum, border=False):
        """ Populates the table with a new particle - a full row """

        for col, (_, attr) in enumerate(self.attrs):
            self.populateAttribute(pnum, col, attr, border)

    #--------------------------------------------------------------------------
    def populateAttribute(self, pnum, col, attr, border=False):
        """ Populates a single cell in the table """

        value = self.data.get(attr, pnum)
        widget = getWidget(value, self.data, attr, pnum)
        if border:
            widget.drawBorder(border)
        self.setCellWidget(pnum, col, widget)
        self.widgets.append(widget)

    #--------------------------------------------------------------------------
    def keyPressEvent(self, event):
        """ Handles certain keys """

        if event.key() in (Qt.Key_Delete, Qt.Key_Backspace):
            self.handleDeleteKey(event)
        else:
            QTableWidget.keyPressEvent(self, event)

    #--------------------------------------------------------------------------
    def handleDeleteKey(self, event): # pylint:disable=W0613
        """ Handles the delete or backspace key """

        model = self.selectionModel()
        rows = model.selectedRows()
        columns = model.selectedColumns()

        if not rows and not columns:
            return

        # Ignore signals as we rebuild
        self.ignoreSignals = True
        if rows:
            particles = [row.row() for row in rows]
            self.data.removeParticles(particles)

        if columns:
            indices = [col.column() for col in columns]
            attributes = [str(self.horizontalHeaderItem(index).text()) for index in indices]
            self.data.removeAttributes(attributes)

        self.ignoreSignals = False
        self.dataResetSlot()


    #--------------------------------------------------------------------------
    def particleAddedSlot(self, index): # pylint:disable=W0613
        """ SLOT when a particle is added """

        if self.ignoreSignals:
            return

        numParticles = self.data.numParticles()
        self.setRowCount(numParticles)
        self.populateParticle(numParticles-1, True)
        self.verticalHeader().resizeSections(QHeaderView.ResizeToContents)

    #--------------------------------------------------------------------------
    def attributeAddedSlot(self, name): # pylint:disable=W0613
        """ SLOT when attribute is added """

        numAttrs = self.data.numAttributes()
        anum = numAttrs - 1
        name = str(name) # partio doesn't like unicode
        attr = self.data.attributeInfo(name)
        self.attrs.append((anum, attr))
        self.setColumnCount(numAttrs)
        self.setHorizontalHeaderItem(numAttrs-1, QTableWidgetItem(attr.name))
        for pnum in range(self.data.numParticles()):
            self.populateAttribute(pnum, anum, attr, True)
        self.verticalHeader().resizeSections(QHeaderView.ResizeToContents)

    #--------------------------------------------------------------------------
    def dataResetSlot(self):
        """ SLOT when particle data is reconstructed """
        if not self.ignoreSignals:
            self.populate()

    #--------------------------------------------------------------------------
    def dataDirtiedSlot(self, dirty):
        """ SLOT when the particle data is dirtied or cleaned.
           When cleaned, reset the style sheets on widgets for border.
        """
        if not dirty:
            for widget in self.widgets:
                widget.drawBorder(False)

#------------------------------------------------------------------------------
class FixedAttributesWidget(QWidget):
    """ A widget for viewing/editing fixed attributes (non-varying) """

    def __init__(self, data, parent=None):
        QWidget.__init__(self, parent)
        self.data = data

        vbox = QVBoxLayout()
        self.setLayout(vbox)
        title = QLabel('Fixed Attributes')
        vbox.addWidget(title)

        self.frame = QFrame()
        vbox.addWidget(self.frame)
        self.vbox = QVBoxLayout()
        self.frame.setLayout(self.vbox)
        self.frame.setFrameShape(QFrame.Panel)
        self.frame.setFrameShadow(QFrame.Sunken)

        self.table = QTableWidget()
        self.table.horizontalHeader().hide()
        self.vbox.addWidget(self.table)
        self.table.hide()

        self.noAttrLabel = QLabel('<i>No fixed attributes</i>')
        self.vbox.addWidget(self.noAttrLabel)


        self.widgets = []
        self.populate()

        self.data.fixedAttributeAdded.connect(self.fixedAttributeAddedSlot)
        self.data.dataReset.connect(self.dataResetSlot)
        self.data.dirtied.connect(self.dataDirtiedSlot)

    def dataDirtiedSlot(self, dirty):
        """ SLOT when the particle data is dirtied or cleaned."""
        if not dirty:
            for widget in self.widgets:
                widget.drawBorder(False)

    def dataResetSlot(self):
        """ SLOT when particle data is reconstructed """
        self.populate()

    def fixedAttributeAddedSlot(self, name): #pylint:disable=W0613
        """ SLOT when a fixed attribute is added to the particle set """
        self.populate()

    def populate(self):
        """ Populates the table of fixed attributes """

        self.widgets = []

        # If no widgets, just drop that in
        numAttrs = self.data.numFixedAttributes()
        if not numAttrs:
            self.table.hide()
            self.noAttrLabel.show()
            return

        self.table.show()
        self.noAttrLabel.hide()
        self.table.setColumnCount(1)
        self.table.setRowCount(numAttrs)
        self.attrs = getAttrs(self.data.numFixedAttributes, self.data.fixedAttributeInfo, True)

        for row, (_, attr) in enumerate(self.attrs):
            item = QTableWidgetItem(attr.name)
            tooltip = '<p><tt>&nbsp;Name: {}<br>&nbsp;Type: {}<br>Count: {}</tt></p>'.\
                      format(attr.name, partio.TypeName(attr.type), attr.count)
            item.setToolTip(tooltip)
            self.table.setVerticalHeaderItem(row, item)
            value = self.data.getFixed(attr)
            widget = getWidget(value, self.data, attr)
            self.table.setCellWidget(row, 0, widget)
            self.widgets.append(widget)
        self.table.horizontalHeader().setStretchLastSection(False)
        self.table.setTabKeyNavigation(True)
        self.table.horizontalHeader().setSectionsMovable(False)

        self.table.horizontalHeader().resizeSections(QHeaderView.ResizeToContents)
        self.table.verticalHeader().resizeSections(QHeaderView.ResizeToContents)


class IndexedStringsWidget(QWidget):
    """ Holds the list of indexed string attributes """
    def __init__(self, data, fixedAttrs=False, parent=None):
        QWidget.__init__(self, parent)
        self.data = data

        vbox = QVBoxLayout()
        self.setLayout(vbox)
        if fixedAttrs:
            title = QLabel('Fixed Attribute Indexed Strings')
        else:
            title = QLabel('Indexed Strings')
        vbox.addWidget(title)
        self.fixedAttrs = fixedAttrs

        self.frame = QFrame()
        vbox.addWidget(self.frame)
        self.vbox = QVBoxLayout()
        self.frame.setLayout(self.vbox)
        self.frame.setFrameShape(QFrame.Panel)
        self.frame.setFrameShadow(QFrame.Sunken)

        self.table = QTableWidget()
        self.table.horizontalHeader().hide()
        self.vbox.addWidget(self.table)
        self.table.hide()

        self.noStringsLabel = QLabel('<i>No indexed strings</i>')
        self.vbox.addWidget(self.noStringsLabel)

        self.populate()

        self.data.dataReset.connect(self.dataResetSlot)

        if fixedAttrs:
            self.data.fixedAttributeAdded.connect(self.attributeAddedSlot)
        else:
            self.data.attributeAdded.connect(self.attributeAddedSlot)

    def getAttributeNum(self):
        if self.fixedAttrs:
            return self.data.numFixedAttributes()
        else:
            return self.data.numAttributes()

    def getAttributeInfo(self, name):
        if self.fixedAttrs:
            return self.data.fixedAttributeInfo(name)
        else:
            return self.data.attributeInfo(name)

    def getIndextedStrs(self, attr):
        if self.fixedAttrs:
            return self.data.fixedIndexedStrs(attr)
        else:
            return self.data.indexedStrs(attr)

    def dataResetSlot(self):
        """ SLOT when particle data is reconstructed """
        self.populate()

    def attributeAddedSlot(self, name): #pylint:disable=W0613
        """ SLOT when an attribute is added to the particle set """
        attr = self.getAttributeInfo(name)
        if attr.type == partio.INDEXEDSTR:
            self.populate()

    def populate(self):
        """ Populates the table of indexed strings """

        attrs = []
        for anum in range(self.getAttributeNum()):
            attr = self.getAttributeInfo(anum)
            if attr.type == partio.INDEXEDSTR:
                attrs.append(attr)

        if not attrs:
            self.table.hide()
            self.noStringsLabel.show()
            return

        self.table.show()
        self.noStringsLabel.hide()
        self.table.setColumnCount(1)
        self.table.setRowCount(len(attrs))

        for row, attr in enumerate(attrs):
            item = QTableWidgetItem(attr.name)
            self.table.setVerticalHeaderItem(row, item)
            strings = self.getIndextedStrs(attr)
            cell_widget = QWidget()
            layout = QVBoxLayout()
            layout.setContentsMargins(0, 0, 0, 0)
            layout.setSpacing(0)
            cell_widget.setLayout(layout)
            table = QTableWidget()
            table.setColumnCount(1)
            table.setMinimumWidth(100)
            table.setRowCount(len(strings))
            table.horizontalHeader().hide()
            table.setVerticalHeaderLabels([str(i) for i in range(len(strings))])
            for i, string in enumerate(strings):
                item = QTableWidgetItem(string)
                table.setItem(i, 0, item)
            table.setFixedHeight(len(strings) * 33)
            table.setWordWrap(False)
            table.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
            table.horizontalHeader().setStretchLastSection(True)
            table.horizontalHeader().resizeSections(QHeaderView.ResizeToContents)
            layout.addWidget(table)
            self.table.setCellWidget(row, 0, cell_widget)

        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.horizontalHeader().setSectionsMovable(False)
        self.table.setTabKeyNavigation(True)

        self.table.horizontalHeader().resizeSections(QHeaderView.ResizeToContents)
        self.table.verticalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.table.verticalHeader().resizeSections(QHeaderView.ResizeToContents)

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
class PartEdit(QMainWindow):
    """ Main window / editor """

    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)

        self.data = ParticleData()

        toolbar = self.addToolBar("Test")

        openButton = QPushButton("")
        openButton.setFlat(True)
        openButton.setIconSize( QSize(32, 32) )
        openButton.setIcon(QIcon("/jobs2/soft/icons/dlight/open.png"))
        openButton.setToolTip( "Open File" )
        toolbar.addWidget(openButton)
        openButton.clicked.connect(self.openSlot)
        QShortcut( QKeySequence(Qt.CTRL + Qt.Key_O), self, self.openSlot )

        saveButton = QPushButton("")
        saveButton.setFlat(True)
        saveButton.setIconSize( QSize(32, 32) )
        saveButton.setIcon(QIcon("/jobs2/soft/icons/dlight/file_save.png"))
        saveButton.setToolTip( "Save File" )
        toolbar.addWidget(saveButton)
        saveButton.clicked.connect(self.saveSlot)
        QShortcut( QKeySequence(Qt.CTRL + Qt.Key_S), self, self.saveSlot )

        saveDeltaButton = QPushButton("")
        saveDeltaButton.setFlat(True)
        saveDeltaButton.setIconSize( QSize(32, 32) )
        saveDeltaButton.setIcon(QIcon("/jobs2/soft/icons/dlight/file_save_as.png"))
        saveDeltaButton.setToolTip( "Save File As Delta" )
        toolbar.addWidget(saveDeltaButton)
        saveDeltaButton.clicked.connect(self.saveDeltaSlot)
        QShortcut( QKeySequence(Qt.CTRL + Qt.SHIFT + Qt.Key_S), self, self.saveDeltaSlot )

        addParticleButton = QPushButton("Particle")
        addParticleButton.setFlat(True)
        addParticleButton.setIconSize( QSize(32, 32) )
        addParticleButton.setIcon(QIcon("/jobs2/soft/icons/shared/plus.png"))
        addParticleButton.setToolTip( "Add Particle" )
        toolbar.addWidget(addParticleButton)
        addParticleButton.clicked.connect(self.addParticleSlot)

        addAttributeButton = QPushButton("Attribute")
        addAttributeButton.setFlat(True)
        addAttributeButton.setIconSize( QSize(32, 32) )
        addAttributeButton.setIcon(QIcon("/jobs2/soft/icons/shared/plus.png"))
        addAttributeButton.setToolTip( "Add Attribute" )
        toolbar.addWidget(addAttributeButton)
        addAttributeButton.clicked.connect(self.addAttributeSlot)

        splitter = QSplitter(self)
        self.setCentralWidget(splitter)

        particleTable = ParticleTableWidget(self.data, self)
        splitter.addWidget(particleTable)
        particleTable.resize(900, particleTable.height())

        right = QWidget(self)
        splitter.addWidget(right)
        vbox = QVBoxLayout(right)
        right.setLayout(vbox)

        fixedAttrWidget = FixedAttributesWidget(self.data, self)
        vbox.addWidget(fixedAttrWidget)

        fixedIndexedStrings = IndexedStringsWidget(self.data, fixedAttrs=True, parent=self)
        vbox.addWidget(fixedIndexedStrings)

        indexedStrings = IndexedStringsWidget(self.data, fixedAttrs=False, parent=self)
        vbox.addWidget(indexedStrings)

        # TODD: SCROLLABLE AREAS FOR EVERYTHING

        self.data.dirtied.connect(self.dataDirtiedSlot)


        # Configure ctrl-w to close the window
        QShortcut( QKeySequence(Qt.CTRL + Qt.Key_W), self, self.close )


    #--------------------------------------------------------------------------
    def openSlot(self):
        """ Callback from Open button """

        # TODO: Check for edits and prompt to save dirty
        if self.data.filename:
            dirname = os.path.dirname(self.data.filename)
        else:
            dirname = os.getcwd()
        filename = QFileDialog.getOpenFileName(self, "Open particle file", dirname, "(*.bgeo *.geo *.bhclassic *.ptc *.pdb)")
        if filename:
            if isinstance(filename, tuple):
                filename = filename[0]
            self.open(str(filename))

    #--------------------------------------------------------------------------
    def open(self, filename):
        """ Opens a file from disk and populates the UI """

        self.data.read(filename)
        self.setWindowTitle(filename)

    #--------------------------------------------------------------------------
    def setData(self, particleSet):
        """ Uses the given particle set as its data """

        self.data.setData(particleSet)

    #--------------------------------------------------------------------------
    def saveSlot(self):
        """ Callback from Save button """
        self.save(False)

    #--------------------------------------------------------------------------
    def saveDeltaSlot(self):
        """ Callback from Save-Delta button """
        self.save(True)

    #--------------------------------------------------------------------------
    def save(self, delta):
        """ Saves the file, either as full or delta """
        if self.data.filename:
            filename = self.data.filename
        else:
            filename = os.getcwd()
        filename = QFileDialog.getSaveFileName(self, "Save particle file", filename,
                                               'Particle Files (*.bgeo *.geo *.bhclassic *.ptc *.pdb );;All files(*)')
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if not filename:
            return
        self.data.write(filename, delta)

    #--------------------------------------------------------------------------
    def addParticleSlot(self):
        """ Adds a new particle (row) to the table """
        self.data.addParticle()

    #--------------------------------------------------------------------------
    def addAttributeSlot(self):
        """ Adds a new attribute (column) to the table """

        dialog = QDialog(self)
        dialog.setModal(True)
        dialog.setWindowTitle('Add Attribute')

        layout = QVBoxLayout()
        dialog.setLayout(layout)

        form = QFormLayout()
        nameBox = QLineEdit()
        typeCombo = QComboBox()
        for attrType in _attrTypes:
            typeName = partio.TypeName(attrType)
            typeCombo.addItem(typeName)
        typeCombo.setCurrentIndex(partio.FLOAT)
        countBox = QLineEdit()
        countBox.setValidator(QIntValidator())
        countBox.setText('1')
        fixedCheckbox = QCheckBox()
        valueBox = QLineEdit()
        valueBox.setText('0')
        form.addRow('Name:', nameBox)
        form.addRow('Type:', typeCombo)
        form.addRow('Count:', countBox)
        form.addRow('Fixed:', fixedCheckbox)
        form.addRow('Default Value:', valueBox)
        layout.addLayout(form)

        buttons = QHBoxLayout()
        layout.addLayout(buttons)

        add = QPushButton('Add')
        add.clicked.connect(dialog.accept)
        buttons.addWidget(add)

        cancel = QPushButton('Cancel')
        cancel.clicked.connect(dialog.reject)
        buttons.addWidget(cancel)

        if not dialog.exec_():
            return

        name = str(nameBox.text())
        if not name:
            print('Please supply a name for the new attribute')  # TODO: prompt
            return

        attrType = typeCombo.currentIndex()
        count = int(countBox.text())
        fixed = fixedCheckbox.isChecked()
        values = list(str(valueBox.text()).strip().split())
        for i in range(count):
            if i < len(values):
                value = values[i]
            else:
                value = values[-1]
            if attrType == partio.INT or attrType == partio.INDEXEDSTR:
                values[i] = int(value)
            elif attrType == partio.FLOAT or attrType == partio.VECTOR:
                values[i] = float(value) # pylint:disable=R0204
            else:
                values[i] = 0.0 # pylint:disable=R0204
        value = tuple(values)

        self.data.addAttribute(name, attrType, count, fixed, value)

    #--------------------------------------------------------------------------
    def dataDirtiedSlot(self, dirty):
        """ Sets the window title with or without "*" for dirty state """
        title = self.data.filename or ''
        if dirty:
            title += '*'
        self.setWindowTitle(title)


#----------------------------------------------------------------------------------------
def styleAppWidgets():
    """Apply widget styling when available"""
    try:
        from minibar.gui import mbWidgetStyling
    except ImportError:
        return
    mbWidgetStyling.styleTheApplication()

#----------------------------------------------------------------------------------------
def main():
    """ Main """

    # Process command-line arguments
    filename = None
    for arg in sys.argv[1:]:
        if arg in ('-h', '--help'):
            print(__doc__)
            sys.exit(0)

        filename = arg

    # Start up the QApplication
    app = QApplication([])
    styleAppWidgets()
    window = PartEdit()
    window.resize(1400, 900)

    # Open file if provided
    if filename:
        window.open(filename)

    window.show()

    # Configure ctrl-q to quit
    QShortcut( QKeySequence(Qt.CTRL + Qt.Key_Q), window, window.close )

    # Go
    app.exec_()


if __name__ == '__main__':
    main()
