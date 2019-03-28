#ifndef XI_H
#define XI_H

namespace xi {

#define INTERFACE(X) virtual void X() = 0;

class Xi {
public:
    INTERFACE(deleteBackward);
    INTERFACE(deleteForward);
    INTERFACE(insertNewline);
    INTERFACE(insertTab);

    INTERFACE(moveLeft);
    INTERFACE(moveWordLeft);
    INTERFACE(moveWordLeftAndModifySelection);
    INTERFACE(moveLeftAndModifySelection);

    INTERFACE(moveRight);
    INTERFACE(moveWordRight);
    INTERFACE(moveWordRightAndModifySelection);
    INTERFACE(moveRightAndModifySelection);

    INTERFACE(moveUp);
    INTERFACE(moveUpAndModifySelection);

    INTERFACE(moveDown);
    INTERFACE(moveDownAndModifySelection);

    INTERFACE(moveToBeginningOfLine);
    INTERFACE(moveToBeginningOfDocumentAndModifySelection);
    INTERFACE(moveToBeginningOfLineAndModifySelection);
    INTERFACE(moveToBeginningOfDocument);

    INTERFACE(moveToEndOfLine);
    INTERFACE(moveToEndOfDocumentAndModifySelection);
    INTERFACE(moveToEndOfLineAndModifySelection);
    INTERFACE(moveToEndOfDocument);

    INTERFACE(scrollPageDown);
    INTERFACE(pageDownAndModifySelection);

    INTERFACE(scrollPageUp);
    INTERFACE(pageUpAndModifySelection);

    INTERFACE(selectAll);

    INTERFACE(uppercase);
    INTERFACE(lowercase);
    INTERFACE(undo);
    INTERFACE(redo);

    INTERFACE(copy);
    INTERFACE(cut);
    INTERFACE(paste);

    //void insertChar(const QString &text);

    Xi();
private:

};

} // namespace xi

#endif // XI_H