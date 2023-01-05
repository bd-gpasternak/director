import functools
import re
import numpy as np
from collections import OrderedDict

from PythonQt import QtGui

from director import callbacks
from director.fieldcontainer import FieldContainer
from director.timercallback import TimerCallback


@functools.lru_cache(maxsize=100)
def cleanPropertyName(s):
    """
    Generate a valid python property name by replacing all non-alphanumeric characters with
    underscores and adding an initial underscore if the first character is a digit
    """
    return re.sub(r'\W|^(?=\d)', '_', s).lower(
    )  # \W matches non-alphanumeric, ^(?=\d) matches the first position if followed by a digit


class PropertyAttributes(FieldContainer):

    def __init__(self, **kwargs):

        self._add_fields(decimals=5, minimum=-1e4, maximum=1e4, singleStep=1, hidden=False,
                         enumNames=None, readOnly=False, docstring="")

        self._set_fields(**kwargs)


def fromQColor(propertyName, propertyValue):
    if isinstance(propertyValue, QtGui.QColor):
        return [
            propertyValue.red() / 255.0,
            propertyValue.green() / 255.0,
            propertyValue.blue() / 255.0
        ]
    else:
        return propertyValue


def toQProperty(propertyName, propertyValue):

    def isNpInstance(value):
        """ Is this a numpy number that needs to be converted back to a default repr """
        return isinstance(value, (np.int, np.float))

    def isIterable(value):
        """ Whether it's an iterable we support, of non-zero length. """
        return isinstance(value, (list, tuple, np.ndarray)) and len(value)

    if isIterable(propertyValue) and isNpInstance(propertyValue[0]):
        propertyValue = [float(x) for x in propertyValue]
        if 'color' in propertyName.lower() and len(propertyValue) == 3:
            return QtGui.QColor(*[255.0 * x for x in propertyValue])
        return propertyValue

    if isNpInstance(propertyValue):
        return float(propertyValue)

    return propertyValue


class PropertySet(object):
    '''PropertySet class provides an interface to a ddPropertiesPanel. This class manages callbacks
    a set of properties (variables) as well as their attributes (settings for a specific variable).
    These properties are displayed in a ddPropertiesPanel. Users changes to variables in the panel
    are reflected in this class, and the proper callbacks are dispatched. Callbacks are fired when
    properties are added, properties change, or property attributes changes. Proper callback
    function signatures as such:

    onPropertyChanged(propertySet, propertyName)
    onPropertyAdded(self, propertySet, propertyName):
    onPropertyAttributeChanged(self, propertySet, propertyName, propertyAttribute)

    Valid properties are basic python types and python lists. There is support for user defined
    enums as well. Properties with "color" in the name are handled as a special case. Internally,
    nested properties would be set and accessed as follows:

    ps = propertyset.PropertySet()
    ps.addProperty('parent/child/nested_bool', True)
    bool_val = ps.getProperty('parent/child/nested_bool')

    Properties can be accessed as members of PropertySet. See cleanPropertyName function for how
    valid python variable names are generated.
    '''

    PROPERTY_CHANGED_SIGNAL = 'PROPERTY_CHANGED_SIGNAL'
    PROPERTY_ADDED_SIGNAL = 'PROPERTY_ADDED_SIGNAL'
    PROPERTY_ATTRIBUTE_CHANGED_SIGNAL = 'PROPERTY_ATTRIBUTE_CHANGED_SIGNAL'

    def __getstate__(self):
        d = dict(_properties=self._properties, _attributes=self._attributes)
        return d

    def __setstate__(self, state):
        self.__init__()
        attrs = state['_attributes']
        for propName, propValue in list(state['_properties'].items()):
            self.addProperty(propName, propValue, attributes=attrs.get(propName))

    def __init__(self):

        self.callbacks = callbacks.CallbackRegistry([
            self.PROPERTY_CHANGED_SIGNAL, self.PROPERTY_ADDED_SIGNAL,
            self.PROPERTY_ATTRIBUTE_CHANGED_SIGNAL
        ])

        self._properties = OrderedDict()
        self._attributes = {}
        self._alternateNames = {}

    def propertyNames(self):
        return list(self._properties.keys())

    def hasProperty(self, propertyName):
        return propertyName in self._properties

    def connectPropertyChanged(self, func):
        return self.callbacks.connect(self.PROPERTY_CHANGED_SIGNAL, func)

    def disconnectPropertyChanged(self, callbackId):
        self.callbacks.disconnect(callbackId)

    def connectPropertyValueChanged(self, propertyName, func):

        def onPropertyChanged(propertySet, changedPropertyName):
            if changedPropertyName == propertyName:
                if self.getPropertyAttributes(propertyName).enumNames:
                    value = propertySet.getPropertyEnumValue(propertyName)
                else:
                    value = propertySet.getProperty(propertyName)
                func(value)

        self.connectPropertyChanged(onPropertyChanged)

    def disconnectPropertyValueChanged(self, callbackId):
        self.callbacks.disconnect(callbackId)

    def connectPropertyAdded(self, func):
        return self.callbacks.connect(self.PROPERTY_ADDED_SIGNAL, func)

    def disconnectPropertyAdded(self, callbackId):
        self.callbacks.disconnect(callbackId)

    def connectPropertyAttributeChanged(self, func):
        return self.callbacks.connect(self.PROPERTY_ATTRIBUTE_CHANGED_SIGNAL, func)

    def disconnectPropertyAttributeChanged(self, callbackId):
        self.callbacks.disconnect(callbackId)

    def getProperty(self, propertyName):
        return self._properties[propertyName]

    def getPropertyEnumValue(self, propertyName):
        attributes = self._attributes[propertyName]
        return attributes.enumNames[self._properties[propertyName]]

    def removeProperty(self, propertyName):
        del self._properties[propertyName]
        del self._attributes[propertyName]
        del self._alternateNames[cleanPropertyName(propertyName)]

    def addProperty(self, propertyName, propertyValue, attributes=None, index=None):
        alternateName = cleanPropertyName(propertyName)
        if propertyName not in self._properties and alternateName in self._alternateNames:
            raise ValueError(
                'Adding this property would conflict with a different existing property with alternate name {:s}'
                .format(alternateName))
        propertyValue = fromQColor(propertyName, propertyValue)
        self._properties[propertyName] = propertyValue
        self._attributes[propertyName] = attributes or PropertyAttributes()
        self._alternateNames[alternateName] = propertyName
        if index is not None:
            self.setPropertyIndex(propertyName, index)
        self.callbacks.process(self.PROPERTY_ADDED_SIGNAL, self, propertyName)

    def setPropertyIndex(self, propertyName, newIndex):
        assert propertyName in self._properties
        assert 0 <= newIndex < len(self._properties)
        value = self._properties.pop(propertyName)
        items = list(self._properties.items())
        items.insert(newIndex, (propertyName, value))
        self._properties = OrderedDict(items)

    def setProperty(self, propertyName, propertyValue):
        previousValue = self._properties[propertyName]
        propertyValue = fromQColor(propertyName, propertyValue)
        names = self.getPropertyAttribute(propertyName, 'enumNames')
        if names and type(propertyValue) != int:
            propertyValue = names.index(propertyValue)
        if propertyValue == previousValue:
            return
        self._properties[propertyName] = propertyValue
        self.callbacks.process(self.PROPERTY_CHANGED_SIGNAL, self, propertyName)

    def getPropertyAttribute(self, propertyName, propertyAttribute):
        attributes = self._attributes[propertyName]
        return attributes[propertyAttribute]

    def getPropertyAttributes(self, propertyName):
        return self._attributes[propertyName]

    def setPropertyAttribute(self, propertyName, propertyAttribute, value):
        attributes = self._attributes[propertyName]
        if attributes[propertyAttribute] != value:
            attributes[propertyAttribute] = value
            self.callbacks.process(self.PROPERTY_ATTRIBUTE_CHANGED_SIGNAL, self, propertyName,
                                   propertyAttribute)

    def __getattribute__(self, name):
        try:
            return object.__getattribute__(self, name)
        except AttributeError as exc:
            alternateNames = object.__getattribute__(self, '_alternateNames')
            if name in alternateNames:
                return object.__getattribute__(self, 'getProperty')(alternateNames[name])
            else:
                raise exc

    def __setattr__(self, name, value):
        try:
            object.__getattribute__(self, name)
        except AttributeError:
            try:
                alternateNames = object.__getattribute__(self, '_alternateNames')
            except AttributeError:
                pass
            else:
                if name in alternateNames:
                    return self.setProperty(alternateNames[name], value)
        object.__setattr__(self, name, value)


class PropertyPanelHelper(object):
    GROUP_SEPARATOR_STR = '-' * 30

    @staticmethod
    def addPropertiesToPanel(properties, panel, propertyNamesToAdd=None):

        for propertyName in properties.propertyNames():
            value = properties.getProperty(propertyName)
            attributes = properties._attributes[propertyName]

            if value is not None and not attributes.hidden:
                addThisProperty = True
                if (propertyNamesToAdd is not None):
                    if (propertyName not in propertyNamesToAdd):
                        addThisProperty = False

                if addThisProperty:
                    PropertyPanelHelper._addProperty(panel, propertyName, attributes, value)

    @staticmethod
    def getPanelProperty(panel, fullPropertyName):
        if "/" in fullPropertyName:
            parent = None
            for name in fullPropertyName.split("/"):
                if parent:
                    prop = panel.getSubProperty(parent, name)
                else:
                    prop = panel.getProperty(name)
                parent = prop
        else:
            prop = panel.getProperty(fullPropertyName)
        return prop

    @staticmethod
    def onPropertyValueChanged(panel, properties, propertyName):
        prop = PropertyPanelHelper.getPanelProperty(panel, propertyName)
        if prop is not None:

            propertyValue = properties.getProperty(propertyName)
            propertyValue = toQProperty(propertyName, propertyValue)

            if isinstance(propertyValue, list):
                for i, subValue in enumerate(propertyValue):
                    panel.getSubProperty(prop, i).setValue(subValue)

                groupName = PropertyPanelHelper.getPropertyGroupName(propertyName, propertyValue)
                prop.setPropertyName(groupName)

            else:
                prop.setValue(propertyValue)

    @staticmethod
    def getFullName(prop, propertiesPanel):
        name = prop.propertyName()
        parent = propertiesPanel.getParentProperty(prop)
        while parent:
            name = parent.propertyName() + "/" + name
            parent = propertiesPanel.getParentProperty(parent)
        return name

    @staticmethod
    def setPropertyFromPanel(prop, propertiesPanel, propertySet):
        if prop.isSubProperty():
            if not propertiesPanel.getParentProperty(prop):
                return
            propertyName = PropertyPanelHelper.getFullName(prop, propertiesPanel)
            propertyIndex = propertiesPanel.getSubPropertyIndex(prop)

            # Special handling for list case vs raw nested params.
            if ('[' in propertyName):
                propertyName = propertyName[:propertyName.index('[') - 1]

                propertyValue = propertySet.getProperty(propertyName)
                propertyValue = list(propertyValue)
                propertyValue[propertyIndex] = prop.value()

                propertySet.setProperty(propertyName, propertyValue)

                varname = propertyName = prop.propertyName()[:prop.propertyName().index('[') - 1]
                groupName = PropertyPanelHelper.getPropertyGroupName(varname, propertyValue)
                propertiesPanel.getParentProperty(prop).setPropertyName(groupName)

            else:
                propertyValue = prop.value()
                propertyValue = fromQColor(propertyName, propertyValue)
                propertySet.setProperty(propertyName, propertyValue)

        else:
            propertyName = prop.propertyName()
            propertyValue = prop.value()
            propertyValue = fromQColor(propertyName, propertyValue)
            propertySet.setProperty(propertyName, propertyValue)

    @staticmethod
    def _setPropertyAttributes(prop, attributes):
        prop.setToolTip(attributes.docstring)
        prop.setEnabled(not attributes.readOnly)
        prop.setAttribute('decimals', attributes.decimals)
        prop.setAttribute('minimum', attributes.minimum)
        prop.setAttribute('maximum', attributes.maximum)
        prop.setAttribute('singleStep', attributes.singleStep)
        if attributes.enumNames:
            prop.setAttribute('enumNames', attributes.enumNames)

    @staticmethod
    def getPropertyGroupName(name, value):
        return '%s [%s]' % (name, ', '.join(
            ['%.2f' % v if isinstance(v, float) else str(v) for v in value]))

    @staticmethod
    def _addProperty(panel, name, attributes, value, parent=None):
        value = toQProperty(name, value)

        # Recursive case
        if "/" in name:
            prefix_suffix = name.split("/", 1)
            if not parent:
                parent = panel.getProperty(prefix_suffix[0])
                parent = parent or panel.addGroup(prefix_suffix[0], prefix_suffix[0])
            else:
                tmp = panel.getSubProperty(parent, prefix_suffix[0])
                parent = tmp or panel.addSubProperty(
                    prefix_suffix[0], PropertyPanelHelper.GROUP_SEPARATOR_STR, parent)
            return PropertyPanelHelper._addProperty(panel, prefix_suffix[1], attributes, value,
                                                    parent=parent)
        # Base cases
        elif isinstance(value, list):
            groupName = PropertyPanelHelper.getPropertyGroupName(name, value)
            if not parent:
                parent = panel.addGroup(name, groupName)
            else:
                parent = panel.addSubProperty(groupName, PropertyPanelHelper.GROUP_SEPARATOR_STR,
                                              parent)
            for i in range(len(value)):
                vis_name = "{} [{}]".format(name, i)
                p = panel.addSubProperty(vis_name, value[i], parent)
                PropertyPanelHelper._setPropertyAttributes(p, attributes)
            return parent
        elif attributes.enumNames:
            if parent:
                p = panel.addEnumSubProperty(name, value, parent)
            else:
                p = panel.addEnumProperty(name, value)
            PropertyPanelHelper._setPropertyAttributes(p, attributes)
            p.setValue(value)
            return p
        else:
            if parent:
                p = panel.addSubProperty(name, value, parent)
            else:
                p = panel.addProperty(name, value)
            PropertyPanelHelper._setPropertyAttributes(p, attributes)
            return p


class PropertyPanelConnector(object):

    def __init__(self, propertySet, propertiesPanel, propertyNamesToAdd=None):
        self.propertySet = propertySet
        self.propertyNamesToAdd = propertyNamesToAdd
        self.propertiesPanel = propertiesPanel
        self.connections = []
        self.connections.append(self.propertySet.connectPropertyAdded(self._onPropertyAdded))
        self.connections.append(self.propertySet.connectPropertyChanged(self._onPropertyChanged))
        self.connections.append(
            self.propertySet.connectPropertyAttributeChanged(self._onPropertyAttributeChanged))
        self.propertiesPanel.connect('propertyValueChanged(QtVariantProperty*)',
                                     self._onPanelPropertyChanged)

        self.timer = TimerCallback()
        self.timer.callback = self._rebuildNow

        self._blockSignals = True
        PropertyPanelHelper.addPropertiesToPanel(self.propertySet, self.propertiesPanel,
                                                 self.propertyNamesToAdd)
        self._blockSignals = False

    def cleanup(self):
        self.timer.callback = None
        self.propertiesPanel.disconnect('propertyValueChanged(QtVariantProperty*)',
                                        self._onPanelPropertyChanged)
        for connection in self.connections:
            self.propertySet.callbacks.disconnect(connection)

    def _rebuild(self):
        if not self.timer.singleShotTimer.isActive():
            self.timer.singleShot(0)

    def _rebuildNow(self):
        self._blockSignals = True
        self.propertiesPanel.clear()
        PropertyPanelHelper.addPropertiesToPanel(self.propertySet, self.propertiesPanel)
        self._blockSignals = False

    def _onPropertyAdded(self, propertySet, propertyName):
        self._rebuild()

    def _onPropertyAttributeChanged(self, propertySet, propertyName, propertyAttribute):
        self._rebuild()

    def _onPropertyChanged(self, propertySet, propertyName):
        self._blockSignals = True
        PropertyPanelHelper.onPropertyValueChanged(self.propertiesPanel, propertySet, propertyName)
        self._blockSignals = False

    def _onPanelPropertyChanged(self, panelProperty):
        if not self._blockSignals:
            PropertyPanelHelper.setPropertyFromPanel(panelProperty, self.propertiesPanel,
                                                     self.propertySet)
