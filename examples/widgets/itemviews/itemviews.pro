TEMPLATE      = subdirs
SUBDIRS       = addressbook \
                basicsortfiltermodel \
                chart \
                coloreditorfactory \
                combowidgetmapper \
                customsortfiltermodel \
                editabletreemodel \
                fetchmore \
                frozencolumn \
                pixelator \
                puzzle \
                simpledommodel \
                simpletreemodel \
                simplewidgetmapper \
                spinboxdelegate \
                spreadsheet \
                stardelegate
!qtConfig(draganddrop): SUBDIRS -= puzzle
!qtHaveModule(xml): SUBDIRS -= simpledommodel
