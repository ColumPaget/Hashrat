for FILE in ../*.c 
do
NAME=`basename $FILE`
cat $FILE | sed "s/AddItemToList/ListAddItem/g" | sed "s/AddNamedItemToList/ListAddNamedItem/g" | sed "s/GetListHead/ListGetHead/g" | sed "s/GetNextListItem/ListGetNext/g" | sed "s/GetPrevListItem/ListGetPrev/g" | sed "s/DeleteNodeFromList/ListDeleteNode/g" | sed "s/ClearList/ListClear/g" | sed "s/DestroyList/ListDestroy/g" | sed "s/CloneList/ListClone/g" | sed "s/GetNthListItem/ListGetNth/g" | sed "s/GetLastListitem/ListGetLast/g" | sed "s/CreateEmptyList/ListCreate/g" | sed "s/CountItemsInList/ListSize/g" | sed "s/InsertItemIntoList/ListInsertItem/g" > $NAME

done
