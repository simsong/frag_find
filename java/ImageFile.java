import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

class ImageFile extends RandomAccessFile {
	public long totalSectors=0;  /* Set when number of sectors is determined */
	public int sectorSize = 512;
	public ImageFile(String fname, String string) throws FileNotFoundException,IOException {
		super(fname, string);
		totalSectors = this.length()/sectorSize;
		if(totalSectors==0){
			/* Compute the number of sectors using binary search.
			 * This will work for up to 2^40 sectors, which is 512TB 
			 * 
			 */
			for(int i=40;i>0;i--){
				long test_sector = totalSectors | (1L<<i);
				try {
					byte[] buf = new byte[1];
					seek(test_sector*sectorSize);
					readFully(buf);
					totalSectors = test_sector;
				} catch (java.io.EOFException e) {
				}
			}
		}
		System.out.println("totalSectors:"+totalSectors);
	}
}
