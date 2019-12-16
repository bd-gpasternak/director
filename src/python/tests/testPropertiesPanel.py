import PythonQt
from PythonQt import QtGui
from director.timercallback import TimerCallback
import director.objectmodel as om
from director import propertyset


class PropertyMonitor(object):

    def __init__(self):
        self.called = False


def startApplication(enableQuitTimer=False):
    appInstance = QtGui.QApplication.instance()
    if enableQuitTimer:
        quitTimer = TimerCallback()
        quitTimer.callback = appInstance.quit
        quitTimer.singleShot(0.1)
    appInstance.exec_()


def main():
    obj = om.ObjectModelItem('test')
    obj.addProperty("parent/child/bool", True)
    obj.addProperty("parent/child/bool2", True)
    obj.addProperty("parent/child/list", [1, 2, 3])
    obj.addProperty("parent/child/nested enum", 2,
                    attributes=om.PropertyAttributes(enumNames=[0, 1, 2]))
    obj.addProperty("parent/child/nested color", [1, 0.0, 0.25])
    obj.addProperty('color', [1.0, 0.5, 0.0])

    obj.addProperty('str list', 1, attributes=om.PropertyAttributes(enumNames=['v0', 'v1', 'v2']))
    obj.addProperty("int list", [1, 2, 3])

    panel = PythonQt.dd.ddPropertiesPanel()
    panel.setBrowserModeToTree()
    panel.show()

    _ = propertyset.PropertyPanelConnector(obj.properties, panel)

    property_monitor = PropertyMonitor()

    def onPropertyChanged(propertySet, propertyName):
        # NOTE this assert does not work in interactive mode.
        # assert not property_monitor.called
        property_monitor.called = True

    obj.properties.connectPropertyChanged(onPropertyChanged)

    # Test enum
    assert obj.getProperty('str list') == 1
    assert obj.getPropertyEnumValue('str list') == 'v1'
    obj.properties.setPropertyAttribute('str list', 'enumNames', ['one', 'two', 'three'])
    assert 'one' in obj.properties.getPropertyAttribute('str list', 'enumNames')
    assert obj.getProperty('str list') == 1
    assert obj.getPropertyEnumValue('str list') == 'two'
    obj.setProperty('str list', 'three')
    assert property_monitor.called
    property_monitor.called = False
    assert obj.getProperty('str list') == 2
    assert obj.getPropertyEnumValue('str list') == 'three'

    # Test nested enum
    assert obj.getProperty('parent/child/nested enum') == 2
    assert obj.getPropertyEnumValue('parent/child/nested enum') == 2
    obj.setProperty('parent/child/nested enum', 0)
    assert property_monitor.called
    property_monitor.called = False
    assert obj.getProperty('parent/child/nested enum') == 0

    # Test nested set / get
    obj.setProperty('parent/child/bool', False)
    assert property_monitor.called
    property_monitor.called = False
    assert not obj.getProperty('parent/child/bool')

    # Test nested list
    obj.setProperty('parent/child/list', [4, 5, 6])
    assert property_monitor.called
    property_monitor.called = False
    assert obj.getProperty('parent/child/list') == [4, 5, 6]

    # Test nested color
    assert obj.getProperty("parent/child/nested color") == [1, 0.0, 0.25]
    obj.setProperty('parent/child/nested color', [0.0, 0.0, 1.0])
    assert property_monitor.called
    property_monitor.called = False
    assert obj.getProperty("parent/child/nested color") == [0.0, 0.0, 1.0]

    om.testObject = obj
    om.testPanel = panel
    _pythonManager.consoleWidget().show()
    startApplication(enableQuitTimer=True)


if __name__ == '__main__':
    main()
