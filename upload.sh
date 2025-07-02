#!/bin/bash

#Mass erase CLI command:
#./STM32_Programmer.sh -vb 3 -c port=swd -e all -el ~/W25Q128JV_STM32U545REQ-NUCLEO_EXTLOADER.stldr

#The built-in verification process doesn't work with large amounts of data so it's not implemented in the loader
#./STM32_Programmer.sh -vb 3 -c port=swd -w ~/testbinary15M79.bin 0x90000000 -v -el ~/W25Q128JV_STM32U545REQ-NUCLEO_EXTLOADER.stldr 
#As a workaround data is read back then compared to the source file to be sure. The maximum amount of read operation possible in one go is 9216 bytes for some reason so it's implemented as reading back chunks and doing the compare. The script stops on the first mismatch and shows the chunk index and byte offset where it failed. On success it shows the total run time of the process and ends.

FLASH_BASE=0x90000000
CHUNK_SIZE=9216 
CUBEPROG_CLI="~/STM32_Programmer.sh"
EXT_LOADER="~/W25Q128JV_STM32U545REQ-NUCLEO_EXTLOADER.stldr"
SOURCE_FILE="~/testfile2K.bin"
LOADED_CHUNK="./tmp_chunk.bin"
SOURCE_CHUNK="./src_chunk.bin"

#Measure total process time
START_TIME=$(date +%s)

# Expand tilde to full path
SOURCE_FILE=$(eval echo "$SOURCE_FILE")
EXT_LOADER=$(eval echo "$EXT_LOADER")

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Source file not found: $SOURCE_FILE"
    exit 1
fi

if [ ! -f "$EXT_LOADER" ]; then
    echo "External loader not found: $EXT_LOADER"
    exit 1
fi

#Write data to the external flash in one go
echo "---Flashing:---"
echo "STM32_Programmer.sh -vb 3 -c port=swd -w $SOURCE_FILE $FLASH_BASE -el \"$EXT_LOADER\""
$CUBEPROG_CLI -vb 3 -c port=swd -w $SOURCE_FILE $FLASH_BASE -el $EXT_LOADER

#Read back and verify chunks on a loop
SOURCE_SIZE=$(stat -c %s "$SOURCE_FILE")
CHUNK_INDEX=0
OFFSET=0

while [ $OFFSET -lt $SOURCE_SIZE ]; do
    BYTES_LEFT=$((SOURCE_SIZE - OFFSET))
    READ_SIZE=$CHUNK_SIZE
    if [ $BYTES_LEFT -lt $CHUNK_SIZE ]; then
        READ_SIZE=$BYTES_LEFT
    fi

    #The CubeProg CLI seems to only accept hex addresses but has no problem with decimal sizes
    FLASH_ADDR=$(printf "0x%X" $((FLASH_BASE + OFFSET)))
    
    #The actual chunk read happens here
    echo "---Reading---:"
    echo "STM32_Programmer.sh -vb 3 -c port=swd -r $FLASH_ADDR $READ_SIZE \"$LOADED_CHUNK\" -el \"$EXT_LOADER\""
    $CUBEPROG_CLI -vb 3 -c port=swd -r $FLASH_ADDR $READ_SIZE "$LOADED_CHUNK" -el "$EXT_LOADER"

    if [ $? -ne 0 ]; then
        printf "Error reading chunk #%d at byte offset 0x%X (@byte %d)\n" "$CHUNK_INDEX" "$OFFSET" "$OFFSET"
        exit 1
    fi

    # Extract the same chunk from source file
    dd if="$SOURCE_FILE" of="$SOURCE_CHUNK" bs=1 skip=$OFFSET count=$READ_SIZE status=none

    #Do the comparison and trigger on a mismatch
    cmp --silent "$LOADED_CHUNK" "$SOURCE_CHUNK"
    if [ $? -ne 0 ]; then
        CMP_OUTPUT=$(cmp "$LOADED_CHUNK" "$SOURCE_CHUNK" 2>&1)
        REL_OFFSET=$(echo "$CMP_OUTPUT" | grep -oP 'byte: \K[0-9]+')
        if [ -z "$REL_OFFSET" ]; then
            echo "Mismatch in chunk #$CHUNK_INDEX, but couldn't extract mismatch offset"
            exit 1
        fi
        ABS_OFFSET=$((OFFSET + REL_OFFSET - 1))
        printf "Mismatch in chunk #%d at byte offset 0x%X (@byte %d)\n" "$CHUNK_INDEX" "$ABS_OFFSET" "$ABS_OFFSET"
        exit 1
    fi

    #Clean up inside curent iteration
    rm "$LOADED_CHUNK" "$SOURCE_CHUNK"
    OFFSET=$((OFFSET + READ_SIZE))
    CHUNK_INDEX=$((CHUNK_INDEX + 1))
done

#Successful exit
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
echo "Flash contents match the source file."
printf "Total time: %d seconds\n" "$ELAPSED"
exit 0
